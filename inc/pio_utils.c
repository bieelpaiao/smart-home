#include "pio_utils.h"

void npInit(uint pin) {
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
    }

    /**
     * Limpa o buffer de pixels.
     */
    void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
    }

    /**
     * Escreve os dados do buffer nos LEDs.
     */
    void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

// int getIndex(int x, int y) {
//     // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
//     // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
//     if (y % 2 == 0) {
//         return y * 5 + x; // Linha par (esquerda para direita).
//     } else {
//         return y * 5 + (4 - x); // Linha ímpar (direita para esquerda).
//     }
// }

int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

int convert100to255(int intensity) {
    return 255 * (intensity/100);
}

void turn_on_leds(int x, int y, int r, int g, int b) {
    int index = getIndex(x, y);
    npSetLED(index, convert100to255(r), convert100to255(g), convert100to255(b));
    // npWrite();
}

void render(int matriz[5][5][3]) {
    // Desenhando Sprite contido na matriz.c
    for(int linha = 0; linha < 5; linha++){
        for(int coluna = 0; coluna < 5; coluna++){
            turn_on_leds(linha, coluna, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
        }
    }
}