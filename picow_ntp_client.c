/**
 * main.c
 * Ponto de entrada do programa com NTP e RTC.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "ntp_utils.h"
#include "hardware/i2c.h"
#include "inc/ssd1306_i2c.h"
#include "inc/ssd1306.h"
// #include "inc/pio_utils.h"
#include "ws2818b.pio.h"

#include "lwip/apps/httpd.h"
#include "ssi.h"
#include "cgi.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "pio_utils.h"


// Definição dos pinos do joystick
#define VRX_PIN 27  // Não usado neste código, mas pode ser usado para navegação horizontal futuramente
#define VRY_PIN 26
#define SW_PIN  22
#define BUTTON_A 6


// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7
#define IS_RGBW false
PIO pio;
uint sm;
uint offset;

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int selected_option = 0;
int selected_option_light = 20;
int count=0;

enum estados {MENU, RELOGIO, ILUMINACAO, CONFIG_PWM_COZINHA, CONFIG_PWM_QUARTO, CONFIG_PWM_SALA, CONFIG_PWM_BANHEIRO} estado;

typedef struct {
    uint8_t *ssd;               // Buffer do display
    struct render_area *frame_area; // Área de renderização
} TimerData;

// Buffer global para armazenar o estado dos LEDs
// static uint32_t pixel_buffer[LED_COUNT] = {0}; // Inicializa todos os LEDs como desligados
 
// Função para enviar um pixel ao PIO (usada internamente)
static inline void send_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Função para atualizar o LED específico no buffer
static inline void put_pixel(PIO pio, uint sm, uint pixel_idx, uint32_t pixel_grb) {
    if (pixel_idx < LED_COUNT) { // Verifica se o índice é válido
        pixel_buffer[pixel_idx] = pixel_grb; // Atualiza apenas o LED especificado no buffer
    }
}

// Função para enviar o buffer completo aos LEDs
static inline void update_pixels(PIO pio, uint sm) {
    for (uint i = 0; i < LED_COUNT; ++i) {
        send_pixel(pio, sm, pixel_buffer[i]); // Envia cada valor do buffer na sequência
    }
}

// Função para criar valores RGB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t) (r) << 8) | ((uint32_t) (g) << 16) | (uint32_t) (b);
}

int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

bool get_pixel_state(int pixel_idx) {
    if (pixel_idx < LED_COUNT) {
        return pixel_buffer[pixel_idx] != 0; // Retorna true se o valor não for 0 (ligado)
    }
    return false; // Índice inválido, assume desligado
}

void npSetLED(const uint x, const uint y, const uint8_t r, const uint8_t g, const uint8_t b) {
    int index = getIndex(x, y);
    put_pixel(pio, sm, index, urgb_u32(r, g, b));
    update_pixels(pio, sm); // Atualiza a faixa com o buffer
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        put_pixel(pio, sm, i, urgb_u32(0, 0, 0));
        update_pixels(pio, sm); // Atualiza a faixa com o buffer
    }       
}

void update_menu(uint8_t *ssd, struct render_area *frame_area, int option) {
    clear_display(ssd, frame_area);
    char *text[] = {"Relogio", "Iluminacao"};

    uint8_t clock_16x16[] = {
        0x07, 0xe0, 0x18, 0x18, 0x27, 0xe4, 0x49, 0x92,
        0x50, 0x0a, 0xa1, 0x85, 0xa1, 0x85, 0xb1, 0xed, 
        0xb1, 0xed, 0xa0, 0x05, 0xa0, 0x05, 0x50, 0x0a,
        0x49, 0x92, 0x27, 0xe4, 0x18, 0x18, 0x07, 0xe0
    };

    uint8_t light_16x16[] = {
        0x07, 0xe0, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04,
        0x28, 0x14, 0x28, 0x14, 0x24, 0x24, 0x22, 0x44, 
        0x12, 0x48, 0x0a, 0x50, 0x06, 0x60, 0x07, 0xe0,
        0x04, 0x20, 0x07, 0xe0, 0x04, 0x20, 0x03, 0xc0
    };

    int y = 8;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 27, y, text[i]);
        y += 24;
    }

    draw_image(ssd, clock_16x16, 6, 4);
    draw_image(ssd, light_16x16, 6, 28);

    draw_selected(ssd, option);

    render_on_display(ssd, frame_area);
}

void update_relogio(uint8_t *ssd, struct render_area *frame_area) {
    clear_display(ssd, frame_area);

    datetime_t dt;
    rtc_get_datetime(&dt);

    // Ajusta para BRT (UTC-3)
    int brt_hour = dt.hour - 3;
    int brt_day = dt.day;
    if (brt_hour < 0) {
        brt_hour += 24;
        brt_day--;
    }

    draw_relogio(ssd, brt_day, dt.month, dt.year, brt_hour, dt.min, dt.sec);
    render_on_display(ssd, frame_area);

    sleep_ms(500);
}
 
bool joystick_button_control_callback(struct repeating_timer *t) {
    static absolute_time_t last_press_time = 0; // Armazena o tempo da última pressão do botão.
    static bool button_last_state = false;      // Armazena o estado anterior do botão (pressionado ou não).

    // Verifica o estado atual do botão (pressionado = LOW, liberado = HIGH).
    bool button_pressed = !gpio_get(SW_PIN); // O botão pressionado gera um nível baixo (0).

    // Verifica se o botão foi pressionado e realiza o debounce.
    if (button_pressed && !button_last_state && 
        absolute_time_diff_us(last_press_time, get_absolute_time()) > 200000) { // Verifica se 200 ms se passaram (debounce).
        last_press_time = get_absolute_time(); // Atualiza o tempo da última pressão do botão.
        button_last_state = true;              // Atualiza o estado do botão como pressionado.
        
        switch (selected_option) {
            case 0:
                count = 0;
                selected_option = 20;
                estado = RELOGIO;
            break;
            case 1:
                count = 0;
                selected_option = 20;
                estado = ILUMINACAO;
            break;
        }

        if (estado == ILUMINACAO) {
            switch (selected_option_light) {
                case 0:
                    count = 0;
                    selected_option_light = 20;
                    estado = CONFIG_PWM_COZINHA;
                    return true;
                break;
                case 1:
                    count = 0;
                    selected_option_light = 20;
                    estado = CONFIG_PWM_QUARTO;
                    return true;
                break;
                case 2:
                    count = 0;
                    selected_option_light = 20;
                    estado = CONFIG_PWM_SALA;
                    return true;
                break;
                case 3:
                    count = 0;
                    selected_option_light = 20;
                    estado = CONFIG_PWM_BANHEIRO;
                    return true;
                break;
            }
            return true;
        }

        if (estado == CONFIG_PWM_COZINHA) {
            printf("Luminosidade da Cozinha Configurada em: %d\n", led_cozinha_level);
            npSetLED(1, 3, 0, 0, 0);
            count = 0;
            estado = MENU;
            return true;
        }

        if (estado == CONFIG_PWM_QUARTO) {
            printf("Luminosidade do Quarto Configurada em: %d\n", led_quarto_level);
            npSetLED(1,1, 0, 0, 0);
            count = 0;
            estado = MENU;
            return true;
        }

        if (estado == CONFIG_PWM_SALA) {
            printf("Luminosidade da Sala Configurada em: %d\n", led_sala_level);
            npSetLED(3,1, 0, 0, 0);
            count = 0;
            estado = MENU;
            return true;
        }

        if (estado == CONFIG_PWM_BANHEIRO) {
            printf("Luminosidade do Banheiro Configurada em: %d\n", led_banheiro_level);
            npSetLED(3,3, 0, 0, 0);
            count = 0;
            estado = MENU;
            return true;
        }
        
    } else if (!button_pressed) {
        button_last_state = false; // Atualiza o estado do botão como liberado quando ele não está pressionado.
    }

    return true; // Retorna true para continuar o temporizador de repetição.
}


bool return_button_control_callback(struct repeating_timer *t) {
    TimerData *data = (TimerData *)t->user_data;
    static absolute_time_t last_press_time = 0; // Armazena o tempo da última pressão do botão.
    static bool button_last_state = false;      // Armazena o estado anterior do botão (pressionado ou não).

    // Verifica o estado atual do botão (pressionado = LOW, liberado = HIGH).
    bool button_pressed = !gpio_get(BUTTON_A); // O botão pressionado gera um nível baixo (0).

    // Verifica se o botão foi pressionado e realiza o debounce.
    if (button_pressed && !button_last_state && 
        absolute_time_diff_us(last_press_time, get_absolute_time()) > 200000) { // Verifica se 200 ms se passaram (debounce).
        last_press_time = get_absolute_time(); // Atualiza o tempo da última pressão do botão.
        button_last_state = true;              // Atualiza o estado do botão como pressionado.
        
        update_menu(data->ssd, data->frame_area, 0);
        count = 0;
        estado = MENU;
        
    } else if (!button_pressed) {
        button_last_state = false; // Atualiza o estado do botão como liberado quando ele não está pressionado.
    }

    return true; // Retorna true para continuar o temporizador de repetição.
}

int main() {
    stdio_init_all();
    // sleep_ms(2000); // Aguarda o USB estabilizar

    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar\n");
        return 1;
    }

    // Obtém o tempo NTP uma vez e configura o RTC
    ntp_fetch_time();

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Inicializa ADC para leitura do joystick
    adc_init();
    adc_gpio_init(VRY_PIN);  // Configura o pino VRy para leitura analógica

    // Configura o botão do joystick como entrada com pull-up interno
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);

    // Configura o botão do joystick como entrada com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    httpd_init();
    printf("Http server initialised\n");

    // Configure SSI and CGI handler
    ssi_init(); 
    printf("SSI Handler initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, LED_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    uint8_t ssd[ssd1306_buffer_length];
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    TimerData timer_data = {
        .ssd = ssd,
        .frame_area = &frame_area
    };

    // zera o display inteiro
    clear_display(ssd, &frame_area);

    update_menu(ssd, &frame_area, selected_option);

    // Configura o temporizador repetitivo para verificar o estado do botão a cada 100 ms.
    struct repeating_timer joystick_button_timer;
    add_repeating_timer_ms(100, joystick_button_control_callback, NULL, &joystick_button_timer);

    struct repeating_timer return_button_timer;
    add_repeating_timer_ms(100, return_button_control_callback, &timer_data, &return_button_timer);

    estado = MENU;

    // Configura o temporizador para exibir o tempo do RTC a cada 1 segundo
    // struct repeating_timer timer;
    // add_repeating_timer_ms(1000, rtc_callback, &timer_data, &timer);

    // Loop principal para outros processos
    while(true) {
        switch (estado) {
            case MENU:
                if (count == 0) {
                    printf("Você está no menu principal\n");
                    selected_option = 0;
                    update_menu(ssd, &frame_area, selected_option);
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                uint16_t vry_value = adc_read();  // Lê o valor do joystick
        
                if (vry_value < 1000) {  // Joystick puxado para baixo
                    if (selected_option < 1) { // Só move se houver outra opção abaixo
                        selected_option++;
                        update_menu(ssd, &frame_area, selected_option);
                    }
                } else if (vry_value > 3000) {  // Joystick empurrado para cima
                    if (selected_option > 0) { // Só move se houver outra opção acima
                        selected_option--;
                        update_menu(ssd, &frame_area, selected_option);
                    }
                }
            break;
            case RELOGIO:
                if (count == 0) {
                    printf("Você está no relógio\n");
                }
                count++;
                update_relogio(ssd, &frame_area);
            break;
            case ILUMINACAO:
                if (count == 0) {
                    draw_menu_light(ssd, &frame_area, 0);
                    printf("Você está no menu de iluminação\n");
                    selected_option_light = 0;
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                vry_value = adc_read();  // Lê o valor do joystick
        
                if (vry_value < 1000) {  // Joystick puxado para baixo
                    if (selected_option_light < 3) { // Só move se houver outra opção abaixo
                        selected_option_light++;
                        draw_menu_light(ssd, &frame_area, selected_option_light);
                    }
                } else if (vry_value > 3000) {  // Joystick empurrado para cima
                    if (selected_option_light > 0) { // Só move se houver outra opção acima
                        selected_option_light--;
                        draw_menu_light(ssd, &frame_area, selected_option_light);
                    }
                }
            break;
            case CONFIG_PWM_COZINHA:
                if (count == 0) {
                    printf("Você está controlando a luminosidade da cozinha\n");
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                vry_value = adc_read();  // Lê o valor do joystick

                // Modo configuração: ajusta intensidade com base no eixo X
                if (vry_value < 1000) {
                    // Joystick à esquerda: diminui intensidade
                    if (led_cozinha_level > 5) led_cozinha_level -= 5;
                } else if (vry_value > 3000) {
                    // Joystick à direita: aumenta intensidade
                    if (led_cozinha_level < 255 - 10) led_cozinha_level += 5;
                }
                npSetLED(1,3, led_cozinha_level, led_cozinha_level, 0);

                // printf("VRX: %d, LED Level: %d\n",
                //     vry_value, led_cozinha_level);
            break;
            case CONFIG_PWM_QUARTO:
                if (count == 0) {
                    printf("Você está controlando a luminosidade do quarto\n");
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                vry_value = adc_read();  // Lê o valor do joystick

                // Modo configuração: ajusta intensidade com base no eixo X
                if (vry_value < 1000) {
                    // Joystick à esquerda: diminui intensidade
                    if (led_quarto_level > 5) led_quarto_level -= 5;
                } else if (vry_value > 3000) {
                    // Joystick à direita: aumenta intensidade
                    if (led_quarto_level < 255 - 10) led_quarto_level += 5;
                }
                npSetLED(1,1, led_quarto_level, led_quarto_level, 0);
            break;
            case CONFIG_PWM_SALA:
                if (count == 0) {
                    printf("Você está controlando a luminosidade da sala\n");
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                vry_value = adc_read();  // Lê o valor do joystick

                // Modo configuração: ajusta intensidade com base no eixo X
                if (vry_value < 1000) {
                    // Joystick à esquerda: diminui intensidade
                    if (led_sala_level > 5) led_sala_level -= 5;
                } else if (vry_value > 3000) {
                    // Joystick à direita: aumenta intensidade
                    if (led_sala_level < 255 - 10) led_sala_level += 5;
                }
                npSetLED(3,1, led_sala_level, led_sala_level, 0);
            break;
            case CONFIG_PWM_BANHEIRO:
                if (count == 0) {
                    printf("Você está controlando a luminosidade do banheiro\n");
                }
                count++;
                adc_select_input(0);  // Seleciona o canal correspondente ao VRy (GPIO26)
                vry_value = adc_read();  // Lê o valor do joystick

                // Modo configuração: ajusta intensidade com base no eixo X
                if (vry_value < 1000) {
                    // Joystick à esquerda: diminui intensidade
                    if (led_banheiro_level > 5) led_banheiro_level -= 5;
                } else if (vry_value > 3000) {
                    // Joystick à direita: aumenta intensidade
                    if (led_banheiro_level < 255 - 10) led_banheiro_level += 5;
                }
                npSetLED(3,3, led_banheiro_level, led_banheiro_level, 0);
            break;
            default:
                printf("Erro");
        }
        
        sleep_ms(200);  // Pequeno delay para evitar leituras rápidas
    }

    cyw43_arch_deinit();
    return 0;
}