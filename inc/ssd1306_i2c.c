#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"
#include "ssd1306_i2c.h"

// Calcular quanto do buffer será destinado à área de renderização
void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

// Processo de escrita do i2c espera um byte de controle, seguido por dados
// void ssd1306_send_command(uint8_t command) {
//     uint8_t buffer[2] = {0x80, command};
//     i2c_write_blocking(i2c1, ssd1306_i2c_address, buffer, 2, false);
// }

void ssd1306_send_command(uint8_t command) {
    uint8_t buffer[2] = {0x80, command};
    int ret = i2c_write_timeout_us(i2c1, ssd1306_i2c_address, buffer, 2, false, 10000); // Timeout de 10ms
    if (ret < 0) {
        // printf("Erro ao enviar comando I2C: %d\n", ret);
    }
}

// Envia uma lista de comandos ao hardware
void ssd1306_send_command_list(uint8_t *ssd, int number) {
    for (int i = 0; i < number; i++) {
        // printf("Enviando comando %d: 0x%02X\n", i, ssd[i]);
        ssd1306_send_command(ssd[i]);
        // printf("Comando %d enviado\n", i);
    }
}

// Copia buffer de referência num novo buffer, a fim de adicionar o byte de controle desde o início
void ssd1306_send_buffer(uint8_t ssd[], int buffer_length) {
    uint8_t *temp_buffer = malloc(buffer_length + 1);

    temp_buffer[0] = 0x40;
    memcpy(temp_buffer + 1, ssd, buffer_length);

    i2c_write_blocking(i2c1, ssd1306_i2c_address, temp_buffer, buffer_length + 1, false);

    free(temp_buffer);
}

// Cria a lista de comandos (com base nos endereços definidos em ssd1306_i2c.h) para a inicialização do display
void ssd1306_init() {
    // printf("Chegou na ssd1306!\n");
    uint8_t commands[] = {
        ssd1306_set_display, ssd1306_set_memory_mode, 0x00,
        ssd1306_set_display_start_line, ssd1306_set_segment_remap | 0x01, 
        ssd1306_set_mux_ratio, ssd1306_height - 1,
        ssd1306_set_common_output_direction | 0x08, ssd1306_set_display_offset,
        0x00, ssd1306_set_common_pin_configuration,
    
#if ((ssd1306_width == 128) && (ssd1306_height == 32))
    0x02,
#elif ((ssd1306_width == 128) && (ssd1306_height == 64))
    0x12,
#else
    0x02,
#endif
        ssd1306_set_display_clock_divide_ratio, 0x80, ssd1306_set_precharge,
        0xF1, ssd1306_set_vcomh_deselect_level, 0x30, ssd1306_set_contrast,
        0xFF, ssd1306_set_entire_on, ssd1306_set_normal_display,
        ssd1306_set_charge_pump, 0x14, ssd1306_set_scroll | 0x00,
        ssd1306_set_display | 0x01,
    };

    // printf("Comandos configurados!\n");

    ssd1306_send_command_list(commands, count_of(commands));
    // printf("Finalizou o ssd1306_init\n");
}

// Cria a lista de comandos para configurar o scrolling
void ssd1306_scroll(bool set) {
    uint8_t commands[] = {
        ssd1306_set_horizontal_scroll | 0x00, 0x00, 0x00, 0x00, 0x03,
        0x00, 0xFF, ssd1306_set_scroll | (set ? 0x01 : 0)
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

// Atualiza uma parte do display com uma área de renderização
void render_on_display(uint8_t *ssd, struct render_area *area) {
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page
    };

    ssd1306_send_command_list(commands, count_of(commands));
    ssd1306_send_buffer(ssd, area->buffer_length);
}

// Determina o pixel a ser aceso (no display) de acordo com a coordenada fornecida
void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set) {
    assert(x >= 0 && x < ssd1306_width && y >= 0 && y < ssd1306_height);

    const int bytes_per_row = ssd1306_width;

    int byte_idx = (y / 8) * bytes_per_row + x;
    uint8_t byte = ssd[byte_idx];

    if (set) {
        byte |= 1 << (y % 8);
    }
    else {
        byte &= ~(1 << (y % 8));
    }

    ssd[byte_idx] = byte;
}

// Algoritmo de Bresenham básico
void ssd1306_draw_line(uint8_t *ssd, int x_0, int y_0, int x_1, int y_1, bool set) {
    int dx = abs(x_1 - x_0); // Deslocamentos
    int dy = -abs(y_1 - y_0);
    int sx = x_0 < x_1 ? 1 : -1; // Direção de avanço
    int sy = y_0 < y_1 ? 1 : -1;
    int error = dx + dy; // Erro acumulado
    int error_2;

    while (true) {
        ssd1306_set_pixel(ssd, x_0, y_0, set); // Acende pixel no ponto atual
        if (x_0 == x_1 && y_0 == y_1) {
            break; // Verifica se o ponto final foi alcançado
        }

        error_2 = 2 * error; // Ajusta o erro acumulado

        if (error_2 >= dy) {
            error += dy;
            x_0 += sx; // Avança na direção x
        }
        if (error_2 <= dx) {
            error += dx;
            y_0 += sy; // Avança na direção y
        }
    }
}

// Adquire os pixels para um caractere (de acordo com ssd1306_font.h)
inline int ssd1306_get_font(uint8_t character)
{
  if (character >= 'A' && character <= 'Z') {
    return character - 'A' + 1;
  }
  else if (character >= '0' && character <= '9') {
    return character - '0' + 27;
  }
  else
    return 0;
}

// Desenha um único caractere no display
void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    y = y / 8;

    character = toupper(character);
    int idx = ssd1306_get_font(character);
    int fb_idx = y * 128 + x;

    for (int i = 0; i < 8; i++) {
        ssd[fb_idx++] = font[idx * 8 + i];
    }
}

// Desenha uma string, chamando a função de desenhar caractere várias vezes
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    while (*string) {
        ssd1306_draw_char(ssd, x, y, *string++);
        x += 8;
    }
}

// Comando de configuração com base na estrutura ssd1306_t
void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
	ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false );
}

// Função de configuração do display para o caso do bitmap
void ssd1306_config(ssd1306_t *ssd) {
    ssd1306_command(ssd, ssd1306_set_display | 0x00);
    ssd1306_command(ssd, ssd1306_set_memory_mode);
    ssd1306_command(ssd, 0x01);
    ssd1306_command(ssd, ssd1306_set_display_start_line | 0x00);
    ssd1306_command(ssd, ssd1306_set_segment_remap | 0x01);
    ssd1306_command(ssd, ssd1306_set_mux_ratio);
    ssd1306_command(ssd, ssd1306_height - 1);
    ssd1306_command(ssd, ssd1306_set_common_output_direction | 0x08);
    ssd1306_command(ssd, ssd1306_set_display_offset);
    ssd1306_command(ssd, 0x00);
    ssd1306_command(ssd, ssd1306_set_common_pin_configuration);
    ssd1306_command(ssd, 0x12);
    ssd1306_command(ssd, ssd1306_set_display_clock_divide_ratio);
    ssd1306_command(ssd, 0x80);
    ssd1306_command(ssd, ssd1306_set_precharge);
    ssd1306_command(ssd, 0xF1);
    ssd1306_command(ssd, ssd1306_set_vcomh_deselect_level);
    ssd1306_command(ssd, 0x30);
    ssd1306_command(ssd, ssd1306_set_contrast);
    ssd1306_command(ssd, 0xFF);
    ssd1306_command(ssd, ssd1306_set_entire_on);
    ssd1306_command(ssd, ssd1306_set_normal_display);
    ssd1306_command(ssd, ssd1306_set_charge_pump);
    ssd1306_command(ssd, 0x14);
    ssd1306_command(ssd, ssd1306_set_display | 0x01);
}

// Inicializa o display para o caso de exibição de bitmap
void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8U;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->bufsize = ssd->pages * ssd->width + 1;
    ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
    ssd->ram_buffer[0] = 0x40;
    ssd->port_buffer[0] = 0x80;
}

// Envia os dados ao display
void ssd1306_send_data(ssd1306_t *ssd) {
    ssd1306_command(ssd, ssd1306_set_column_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->width - 1);
    ssd1306_command(ssd, ssd1306_set_page_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->pages - 1);
    i2c_write_blocking(
    ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false );
}

// Desenha o bitmap (a ser fornecido em display_oled.c) no display
void ssd1306_draw_bitmap(ssd1306_t *ssd, const uint8_t *bitmap) {
    for (int i = 0; i < ssd->bufsize - 1; i++) {
        ssd->ram_buffer[i + 1] = bitmap[i];

        ssd1306_send_data(ssd);
    }
}

// Função para desenhar uma imagem genérica no framebuffer
void draw_image(uint8_t *ssd,  uint8_t *image, int x_start, int y_start) {
    for (int y = 0; y < 16; y++) {   // Percorre as 16 linhas
        for (int x = 0; x < 16; x++) { // Percorre as 16 colunas
            int byte_index = (y * 2) + (x / 8); // Cada linha tem 2 bytes
            int bit_index = 7 - (x % 8); // Pega o bit correto no byte

            bool pixel_on = (image[byte_index] >> bit_index) & 1;
            ssd1306_set_pixel(ssd, x_start + x, y_start + y, pixel_on);
        }
    }
}

void draw_selected(uint8_t *ssd, int option) {
    int i = option*24;

    ssd1306_draw_line(ssd, 1, 2+i, 3, 0+i, true);
    ssd1306_draw_line(ssd, 1, 21+i, 3, 23+i, true);
    ssd1306_draw_line(ssd, 1, 3+i, 1, 20+i, true);
    ssd1306_draw_line(ssd, 124, 0+i, 126, 2+i, true);
    ssd1306_draw_line(ssd, 126, 3+i, 126, 20+i, true);
    ssd1306_draw_line(ssd, 124, 23+i, 126, 21+i, true);
    ssd1306_draw_line(ssd, 4, 0+i, 124, 0+i, true);
    ssd1306_draw_line(ssd, 4, 23+i, 124, 23+i, true);
}

void draw_4_selected(uint8_t *ssd, int option) {
    int i = option*16;

    ssd1306_draw_line(ssd, 1, 2+i, 3, 0+i, true);
    ssd1306_draw_line(ssd, 1, 13+i, 3, 15+i, true);
    ssd1306_draw_line(ssd, 1, 3+i, 1, 12+i, true);

    ssd1306_draw_line(ssd, 124, 0+i, 126, 2+i, true);
    ssd1306_draw_line(ssd, 126, 3+i, 126, 12+i, true);
    ssd1306_draw_line(ssd, 124, 15+i, 126, 13+i, true);

    ssd1306_draw_line(ssd, 4, 0+i, 123, 0+i, true);
    ssd1306_draw_line(ssd, 4, 15+i, 123, 15+i, true);
}

char* int_to_two_digit_str(int value) {
    // Aloca memória para a string (2 algarismos + terminador nulo)
    char *str = (char *)malloc(3 * sizeof(char));
    if (str == NULL) {
        printf("Erro ao alocar memória para string\n");
        return NULL; // Retorna NULL em caso de falha
    }

    // Formata o inteiro como string de 2 algarismos
    snprintf(str, 3, "%02d", value);

    return str;
}

void draw_relogio(uint8_t *ssd, int day, int month, int year, int hour, int min, int sec) {
    char buffer [33];

    char *text[] = {
        "Data",
        "Hora"};

    int y = 8;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 24;
    }

    ssd1306_set_pixel(ssd, 37, 8, true);
    ssd1306_set_pixel(ssd, 37, 14, true);
    ssd1306_set_pixel(ssd, 37, 32, true);
    ssd1306_set_pixel(ssd, 37, 38, true);
    ssd1306_draw_string(ssd, 41, 8, int_to_two_digit_str(day));
    ssd1306_draw_line(ssd, 57, 14, 59, 8, true);
    ssd1306_draw_string(ssd, 61, 8, int_to_two_digit_str(month));
    ssd1306_draw_line(ssd, 77, 14, 79, 8, true);
    ssd1306_draw_string(ssd, 81, 8, int_to_two_digit_str(year));
    ssd1306_draw_string(ssd, 41, 32, int_to_two_digit_str(hour));
    ssd1306_set_pixel(ssd, 57, 32, true);
    ssd1306_set_pixel(ssd, 57, 38, true);
    ssd1306_draw_string(ssd, 61, 32, int_to_two_digit_str(min));
    ssd1306_set_pixel(ssd, 77, 32, true);
    ssd1306_set_pixel(ssd, 77, 38, true);
    ssd1306_draw_string(ssd, 81, 32, int_to_two_digit_str(sec));
}

void clear_display(uint8_t *ssd, struct render_area *frame_area) {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);
}

void draw_menu_light(uint8_t *ssd, struct render_area *frame_area, int option) {
    clear_display(ssd, frame_area);
    char *text[] = {"Cozinha", "Quarto", "Sala", "Banheiro"};

    // 'bedroom', 16x16px
    uint8_t bitmap_bedroom [] = {
        0x00, 0x00, 0xe0, 0x00, 0xa0, 0x00, 0xb8, 0x00,
        0xaf, 0xfc, 0xa8, 0x02, 0xa8, 0x01, 0xa8, 0x01, 
        0xa8, 0x01, 0xbf, 0xff, 0x80, 0x01, 0x80, 0x01,
        0x80, 0x01, 0x9f, 0xfd, 0xe0, 0x07, 0x00, 0x00
    };
    // 'kitchen', 16x16px
    uint8_t bitmap_kitchen [] = {
        0x2a, 0x1c, 0x2a, 0x24, 0x2a, 0x24, 0x2a, 0x24,
        0x2a, 0x24, 0x1c, 0x24, 0x08, 0x24, 0x1c, 0x24, 
        0x14, 0x34, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
        0x14, 0x14, 0x14, 0x14, 0x1c, 0x1c, 0x08, 0x08
    };
    // 'living_room', 16x16px
    uint8_t bitmap_living_room [] = {
        0x00, 0x00, 0x3f, 0xfc, 0x20, 0x04, 0x35, 0x54,
        0x2a, 0xa4, 0x60, 0x06, 0x7f, 0xfe, 0x61, 0x86, 
        0x61, 0x86, 0x61, 0x86, 0x7f, 0xfe, 0x40, 0x02,
        0x7f, 0xfe, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00
    };
    // 'toilet', 16x16px
    uint8_t bitmap_toilet [] = {
        0x1f, 0xf8, 0x20, 0x24, 0x40, 0x42, 0x40, 0x5a,
        0x80, 0x99, 0x80, 0x99, 0x80, 0x99, 0x80, 0x99, 
        0x80, 0x5a, 0x80, 0x42, 0x80, 0x64, 0x40, 0x78,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3f, 0xc0
    };



    int y = 8;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 27, y, text[i]);
        y += 16;
    }

    draw_image(ssd, bitmap_kitchen, 6, 2);
    draw_image(ssd, bitmap_bedroom, 6, 16);
    draw_image(ssd, bitmap_living_room, 6, 32);
    draw_image(ssd, bitmap_toilet, 6, 48);

    draw_4_selected(ssd, option);

    render_on_display(ssd, frame_area);
}