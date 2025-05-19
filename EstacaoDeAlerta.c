#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "blink.pio.h"
#include "lib/ssd1306.h"
#include "lib/matriz.h"


#define WS2812_PIN 7 

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

#define JOY_X_ADC 1
#define JOY_Y_ADC 0
#define JOY_GPIO_ADX 27
#define JOY_GPIO_ADY 26

#define BUZZER_PIN 21
#define LED_PIN_GREEN 11
#define LED_PIN_RED 13

// Thresholds para alerta
#define LEVEL_THRESHOLD 0.70f // 70% do escopo ADC
#define RAIN_THRESHOLD 0.80f  // 80% do escopo ADC

// Estrutura para dados do joystick
typedef struct
{
    uint16_t nivel_da_agua;
    uint16_t volume_de_chuva;
} SensorData_t;

// Fila para comunicacaoo de dados
static QueueHandle_t xSensorQueue;

// Flag que determina modo de operacaoo: true = Normal, false = Alerta
static volatile bool modoNormal = true;

// Variaveis globais para os ultimos valores lidos, usados pelo Display
static volatile uint16_t lastNivel = 0;
static volatile uint16_t lastVolume = 0;

// ---------- Prototipos das tarefas ----------
void vTaskBuzzer(void *pvParameters);
void vTaskJoystick(void *pvParameters);
void vTaskProcessamento(void *pvParameters);
void vTaskRGB(void *pvParameters);
void vTaskDisplay(void *pvParameters);
void vTaskMatriz(void *pvParameters);

int main()
{
    stdio_init_all();

    // Inicializa ADC para joystick
    adc_init();
    adc_gpio_init(JOY_GPIO_ADX);
    adc_gpio_init(JOY_GPIO_ADY);

    // Inicializa PWM para LEDs e buzzer
    gpio_set_function(LED_PIN_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_RED, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_red = pwm_gpio_to_slice_num(LED_PIN_RED);
    uint slice_green = pwm_gpio_to_slice_num(LED_PIN_GREEN);
    pwm_set_clkdiv(slice_red, 500.0f);
    pwm_set_wrap(slice_red, 255);
    pwm_set_clkdiv(slice_green, 500.0f);
    pwm_set_wrap(slice_green, 255);
    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);
    pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true);

    // Inicializa OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Cria fila de dados do joystick (tamanho 5 elementos)
    xSensorQueue = xQueueCreate(5, sizeof(SensorData_t));

    // Criacao das tarefas 
    xTaskCreate(vTaskBuzzer, "Buzzer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vTaskJoystick, "Joystick", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vTaskProcessamento, "Processamento", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(vTaskRGB, "LED RGB", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vTaskDisplay, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vTaskMatriz, "Matriz", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
    return 0;
}

// ---------- Implementacao das tarefas ----------

/* Tarefa para tocar o buzzer com pwm. No modo normal, apenas desliga o buzzer para se certificar que nada
 * esta tocando. No modo alerta, apresenta um ruido sonoro intermitente. */
void vTaskBuzzer(void *pvParameters)
{
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice, true);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);
    uint wrap = 125000000 / 3500;
    pwm_set_wrap(slice, wrap);

    while (true)
    {
        if (modoNormal)
        {
            pwm_set_enabled(slice, false);
            vTaskDelay(pdMS_TO_TICKS(500));
            wrap = 125000000 / 2000;
            pwm_set_chan_level(slice, chan, 0);
        }
        else
        {
            pwm_set_enabled(slice, true);
            for (int i = 0; i < 3 && !modoNormal; ++i)
            {
                pwm_set_chan_level(slice, chan, wrap / 2);
                vTaskDelay(pdMS_TO_TICKS(250));
                pwm_set_chan_level(slice, chan, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
        }
    }
}

/* Tarefa para a matriz de LEDs. No modo normal, apenas acende todos os LEDs na cor verde. No modo alerta
 * exibe simbolos como "X" e "!" para alerta visual. */
void vTaskMatriz(void *pvParameters)
{
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    blink_program_init(pio, sm, offset, WS2812_PIN);

    // Vetor com os desenhos que serao exibidos na matriz. Todos eles estao desenhados em lib/matriz.c
    double *modos[3] = {modoNormalMatriz, modoAlertaMatriz1, modoAlertaMatriz2};

    while (true)
    {
        if (modoNormal)
        {
            // Acende todos os LEDs na cor verde
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = modos[0][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            // Todos os LEDs da matriz brilhando lentamente. Alternando entre "X" e "!".
            for (float brightness = 0.0; brightness <= 1.0 && !modoNormal; brightness += 0.05)
            {
                for (int i = 0; i < NUM_PIXELS; i++)
                {
                    double intensity = modos[2][24 - i];
                    unsigned char R = (unsigned char)(intensity * brightness * 255);
                    unsigned char G = 0;
                    unsigned char B = 0;
                    uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                    pio_sm_put_blocking(pio, sm, valor_led);
                }
                vTaskDelay(pdMS_TO_TICKS(63.75));
            }
            for (float brightness = 1.0; brightness >= 0.0 && !modoNormal; brightness -= 0.05)
            {
                for (int i = 0; i < NUM_PIXELS; i++)
                {
                    double intensity = modos[1][24 - i];
                    unsigned char R = (unsigned char)(intensity * brightness * 255);
                    unsigned char G = 0;
                    unsigned char B = 0;
                    uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                    pio_sm_put_blocking(pio, sm, valor_led);
                }
                vTaskDelay(pdMS_TO_TICKS(63.75));
            }
        }
    }
}

/* Tarefa para leitura do joystick e envio à fila. O nivel da agua eh lido pelo adc no eixo X e o volume de
 * chuva eh lido pelo adc no eixo Y. */
void vTaskJoystick(void *pvParameters)
{
    SensorData_t data;
    while (1)
    {
        adc_select_input(JOY_X_ADC);
        data.nivel_da_agua = adc_read();
        adc_select_input(JOY_Y_ADC);
        data.volume_de_chuva = adc_read();
        xQueueSend(xSensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Processamento central: recebe os dados do joystick, atualiza o modo e as variaveis globais
void vTaskProcessamento(void *pvParameters)
{
    SensorData_t recebido;
    const float maxAdc = 4095.0f;
    while (1)
    {
        if (xQueueReceive(xSensorQueue, &recebido, portMAX_DELAY) == pdTRUE)
        {
            lastNivel = recebido.nivel_da_agua;
            lastVolume = recebido.volume_de_chuva;
            float pctNivel = recebido.nivel_da_agua / maxAdc;
            float pctChuva = recebido.volume_de_chuva / maxAdc;
            // Alerta se ultrapassar thresholds
            modoNormal = !(pctNivel >= LEVEL_THRESHOLD || pctChuva >= RAIN_THRESHOLD);
        }
    }
}

/* Tarefa para alternar entre os LEDs RGB com PWM. No modo normal apenas o LED verde acende, no modo alerta
*  apenas o LED vermelho acende, oscilando lentamente com PWM. */
void vTaskRGB(void *pvParameters)
{
    while (true)
    {
        pwm_set_gpio_level(LED_PIN_GREEN, 0);
        pwm_set_gpio_level(LED_PIN_RED, 0);

        // LED verde
        if (modoNormal)
        {
            pwm_set_gpio_level(LED_PIN_GREEN, 255);
        }
        else
        {
            // LED vermelho
            pwm_set_gpio_level(LED_PIN_GREEN, 0);
            for (uint brightness = 0; brightness <= 255 && !modoNormal; brightness++)
            {
                pwm_set_gpio_level(LED_PIN_RED, brightness);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            for (int brightness = 255; brightness >= 0 && !modoNormal; brightness--)
            {
                pwm_set_gpio_level(LED_PIN_RED, brightness);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Tarefa do display OLED. Exibe os valores do nivel da agua e do volume de chuva em porcentagem. Caso o
 * nivel da agua ultrapasse os 70% ou o volume de chuva ultrapasse os 80%, exibe uma mensagem de alerta.*/
void vTaskDisplay(void *pvParameters)
{
    const float maxAdc = 4095.0f;
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    while (1)
    {
        ssd1306_fill(&ssd, false);
        char buf[32];
        float pctNivel = (lastNivel / maxAdc) * 100.0f;
        float pctChuva = (lastVolume / maxAdc) * 100.0f;
        // Exibe niveis
        sprintf(buf, "N. de agua:%3.0f%%", pctNivel);
        ssd1306_draw_string(&ssd, buf, 0, 0);
        sprintf(buf, "V. de chuva:%3.0f%%", pctChuva);
        ssd1306_draw_string(&ssd, buf, -1, 16);
        // Indica modo
        if (!modoNormal)
        {
            ssd1306_draw_string(&ssd, " ALERTA! ", 27, 47);
        }
        ssd1306_send_data(&ssd);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}