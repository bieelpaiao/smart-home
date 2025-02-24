#ifndef PIO_UTILS_H
#define PIO_UTILS_H

#include <stdbool.h> // Para usar o tipo bool

#define LED_COUNT 25

// Buffer global para armazenar o estado dos LEDs
uint32_t pixel_buffer[LED_COUNT] = {0}; // Inicializa todos os LEDs como desligados

uint16_t led_cozinha_level = 5;     // Nível inicial do PWM (meio da escala)
uint16_t led_quarto_level = 5;     // Nível inicial do PWM (meio da escala)
uint16_t led_sala_level = 5;     // Nível inicial do PWM (meio da escala)
uint16_t led_banheiro_level = 5;     // Nível inicial do PWM (meio da escala)

bool get_pixel_state(int pixel_idx);
int getIndex(int x, int y);
void npSetLED(const uint x, const uint y, const uint8_t r, const uint8_t g, const uint8_t b);
#endif
