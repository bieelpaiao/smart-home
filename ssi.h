#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "pio_utils.h"

#define LED_R 13
#define LED_G 11
#define LED_B 12

// SSI tags - tag length limited to 8 bytes by default
const char * ssi_tags[] = {"cozinha", "sala", "quarto", "banheiro", "c_int", "s_int", "q_int", "b_int"};

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
  size_t printed;
  switch (iIndex) {
  case 0: // cozinha
    {
      bool led_status = get_pixel_state(getIndex(1,3));
      if(led_status == true){
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room3 lit\"><a href=\"/led.cgi?cozinha=0\" class=\"lamp on\" id=\"cozinha\"></a>");
      }
      else{
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room3\"><a href=\"/led.cgi?cozinha=0\" class=\"lamp\" id=\"cozinha\"></a>");
      }
    }
    break;
  case 1: // sala
    {
      bool led_status = get_pixel_state(getIndex(3,1));
      if(led_status == true){
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room2 lit\"><a href=\"/led.cgi?sala=0\" class=\"lamp on\" id=\"sala\"></a>");
      }
      else{
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room2\"><a href=\"/led.cgi?sala=0\" class=\"lamp\" id=\"sala\"></a>");
      }
    }
    break;
  case 2: // quarto
    {
      bool led_status = get_pixel_state(getIndex(1,1));
      if(led_status == true){
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room1 lit\"><a href=\"/led.cgi?quarto=0\" class=\"lamp on\" id=\"quarto\"></a>");
      }
      else{
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room1\"><a href=\"/led.cgi?quarto=0\" class=\"lamp\" id=\"quarto\"></a>");
      }
    }
    break;
    case 3: // banheiro
    {
      bool led_status = get_pixel_state(getIndex(3,3));
      if(led_status == true){
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room4 lit\"><a href=\"/led.cgi?banheiro=0\" class=\"lamp on\" id=\"banheiro\"></a>");
      }
      else{
        printed = snprintf(pcInsert, iInsertLen, "<div class=\"room room4\"><a href=\"/led.cgi?banheiro=0\" class=\"lamp\" id=\"banheiro\"></a>");
      }
    }
    break;
    case 4:
      printed = snprintf(pcInsert, iInsertLen, "%.0f", 100.0*(led_cozinha_level/255.0));
    break;
    case 5:
      printed = snprintf(pcInsert, iInsertLen, "%.0f", 100.0*(led_sala_level/255.0));
    break;
    case 6:
      printed = snprintf(pcInsert, iInsertLen, "%.0f", 100.0*(led_quarto_level/255.0));
    break;
    case 7:
      printed = snprintf(pcInsert, iInsertLen, "%.0f", 100.0*(led_banheiro_level/255.0));
    break;
  default:
    printed = 0;
    break;
  }

  return (u16_t)printed;
}

// Initialise the SSI handler
void ssi_init() {
  gpio_init(LED_R);
  gpio_set_dir(LED_R, GPIO_OUT);
  gpio_put(LED_R, 0); // Garante que o LED comece apagado.

  gpio_init(LED_G);
  gpio_set_dir(LED_G, GPIO_OUT);
  gpio_put(LED_G, 0); // Garante que o LED comece apagado.

  gpio_init(LED_B);
  gpio_set_dir(LED_B, GPIO_OUT);
  gpio_put(LED_B, 0); // Garante que o LED comece apagado.
  http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}