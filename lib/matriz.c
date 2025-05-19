#include "matriz.h"


// Todos os LEDs acesos, para o modo normal
double modoNormalMatriz[NUM_PIXELS] = {0.6, 0.6, 0.6, 0.6, 0.6,
                                       0.6, 0.6, 0.6, 0.6, 0.6,
                                       0.6, 0.6, 0.6, 0.6, 0.6,
                                       0.6, 0.6, 0.6, 0.6, 0.6,
                                       0.6, 0.6, 0.6, 0.6, 0.6};

// Forma um "!" na matriz
double modoAlertaMatriz1[NUM_PIXELS] = {0.0, 0.0, 1.0, 0.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0, 0.0,
                                        0.0, 0.0, 0.0, 0.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0, 0.0};

// Forma um "X" na matriz
double modoAlertaMatriz2[NUM_PIXELS] = {1.0, 0.0, 0.0, 0.0, 1.0,
                                        0.0, 1.0, 0.0, 1.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0, 0.0,
                                        0.0, 1.0, 0.0, 1.0, 0.0,
                                        1.0, 0.0, 0.0, 0.0, 1.0};


