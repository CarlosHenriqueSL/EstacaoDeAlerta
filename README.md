# Estação de alerta com a Raspberry Pi Pico W
**EMBARCATECH - Fase 2**

**Sistema de alerta inteligente usando FreeRTOS e a BitDogLab**

## Desenvolvedor
- **Carlos Henrique Silva Lopes**

---

### 📄 Descrição

Este é um projeto que simula uma estação de alerta de enchente uma placa **BitDogLab (RP2040)**, utilizando o **FreeRTOS** para gerenciar tarefas concorrentes. O sistema opera em dois modos:

* **Modo Normal:** Feedback visual com LEDs RGB e também na matriz de LEDs, exibição de valores no display SSD1306.
* **Modo Alerta:** Apenas a luz vermelha (LED RGB e matriz de LEDs) pisca suavemente com PWM, e o buzzer emite um beep intermitente,
o display exibe os valores de nível da água e volume de chuva, assim como uma mensagem de alerta.

A troca de modos é feita movendo-se o joystick da placa BitDogLab, onde o ADC do Eixo X faz a leitura do nível da água, e o ADC do Eixo Y faz a leitura do nível da chuva.

---

### 🎯 Objetivo Geral

Utilizar tarefas do **FreeRTOS** para controlar LEDs, buzzer, matriz WS2812, display SSD1306 e joystick.

---

### ⚙️ Funcionalidades

* **Controle de LEDs RGB:** Verde no modo normal; brilho oscilante em vermelho no modo alerta.
* **Matriz WS2812:** Sinaliza com a luz verde que o sistema está em modo normal e simula PWM para modo alerta.
* **Display SSD1306:** Apresenta os valores lidos nos dois modos, para o modo alerta adiciona uma mensagem abaixo dos valores.
* **Buzzer *(PWM)*:** Desligado no modo normal; beep intermitente no modo alerta.
* **Joystick:** Faz a leitura dos valores e alterna entre estados.

---

### 📌 Mapeamento de Pinos

| Função             | GPIO |
| ------------------ | ---- |
| WS2812 (Matriz)    | 7    |
| LED Verde (PWM)    | 11   |
| LED Vermelho (PWM) | 13   |
| Buzzer (PWM)       | 21   |

---

### 🛠️ Estrutura do Código

* **vTaskRGB**: Gerencia LEDs RGB via PWM.
* **vTaskMatriz**: Controla a matriz WS2812
* **vTaskDisplay**: Inicializa e atualiza o SSD1306 via I2C.
* **vTaskBuzzer**: Emite sons com PWM conforme o modo
* **vTaskJoystick**: Faz a leitura com ADC e converte em porcentagem.
* **vTaskProcessamento**: Processa os dados armazenados na fila.

---

## Estrutura do Código

### Principais Arquivos
- **`EstacaoDeAlerta.c`**: Contém a lógica principal do programa, com todas as tarefas.
- **`lib/`**: Contém os arquivos com a lógica principal para desenhar no display ssd1306, também contém os arquivos com os desenhos da matriz de LEDs e para a configuração do FreeRTOS.
- **`lib/matriz.c`**:  Contém os desenhos que serão feitos na matriz de LEDs.
- **`lib/ssd1306.c`**: Contém as funções para desenhar no display ssd1306.
- **`blink.pio`**: Contém a configuração em Assembly para funcionamento do pio.
- **`README.md`**: Documentação detalhada do projeto.
