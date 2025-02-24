#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "pio_utils.h"

// CGI handler which is run when a request for /led.cgi is detected
const char * cgi_led_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for (int i=0; i<iNumParams; i++) {
        // Check if an request for LED has been made (/led.cgi?led=x)
        if (strcmp(pcParam[i] , "cozinha") == 0){
            // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
            if(strcmp(pcValue[i], "0") == 0)
                npSetLED(1, 3, 0, 0, 0);
            else if(strcmp(pcValue[0], "1") == 0)
                npSetLED(1, 3, led_cozinha_level, led_cozinha_level, 0);
        }

        if (strcmp(pcParam[i] , "sala") == 0){
            // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
            if(strcmp(pcValue[i], "0") == 0)
                npSetLED(3, 1, 0, 0, 0);
            else if(strcmp(pcValue[0], "1") == 0)
                npSetLED(3, 1, led_sala_level, led_sala_level, 0);
        }

        if (strcmp(pcParam[i] , "quarto") == 0){
            // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
            if(strcmp(pcValue[i], "0") == 0)
                npSetLED(1, 1, 0, 0, 0);
            else if(strcmp(pcValue[0], "1") == 0)
                npSetLED(1, 1, led_quarto_level, led_quarto_level, 0);
        }

        if (strcmp(pcParam[i] , "banheiro") == 0){
            // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
            if(strcmp(pcValue[i], "0") == 0)
                npSetLED(3, 3, 0, 0, 0);
            else if(strcmp(pcValue[0], "1") == 0)
                npSetLED(3, 3, led_banheiro_level, led_banheiro_level, 0);
        }
    }
    
    
    // Send the index page back to the user
    return "/index.shtml";
}

// tCGI Struct
// Fill this with all of the CGI requests and their respective handlers
static const tCGI cgi_handlers[] = {
    {
        // Html request for "/led.cgi" triggers cgi_handler
        "/led.cgi", cgi_led_handler
    },
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers, 1);
}