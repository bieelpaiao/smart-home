/**
 * ntp_utils.c
 * Implementações para o cliente NTP simplificado e RTC.
 */

 #include <string.h>
 #include <time.h>
 #include "ntp_utils.h"
 
 NTP_T* ntp_init(void) {
     NTP_T *state = (NTP_T*)calloc(1, sizeof(NTP_T));
     if (!state) {
         printf("Falha ao alocar estado\n");
         return NULL;
     }
     state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
     if (!state->ntp_pcb) {
         printf("Falha ao criar PCB\n");
         free(state);
         return NULL;
     }
     udp_recv(state->ntp_pcb, ntp_recv, state);
     state->epoch = 0;
     return state;
 }
 
 void ntp_request(NTP_T *state) {
     cyw43_arch_lwip_begin();
     struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
     uint8_t *req = (uint8_t *)p->payload;
     memset(req, 0, NTP_MSG_LEN);
     req[0] = 0x1b;
     udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
     pbuf_free(p);
     cyw43_arch_lwip_end();
 }
 
 void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
     NTP_T *state = (NTP_T*)arg;
     if (ipaddr) {
         state->ntp_server_address = *ipaddr;
         printf("Endereço NTP resolvido: %s\n", ipaddr_ntoa(ipaddr));
         ntp_request(state);
     } else {
         printf("Falha na resolução DNS do NTP\n");
     }
 }
 
 void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
     NTP_T *state = (NTP_T*)arg;
     uint8_t mode = pbuf_get_at(p, 0) & 0x7;
     uint8_t stratum = pbuf_get_at(p, 1);
 
     if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
         mode == 0x4 && stratum != 0) {
         uint8_t seconds_buf[4] = {0};
         pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
         uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
         uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
         state->epoch = seconds_since_1970;
         printf("Tempo NTP obtido: %ld segundos desde 1970\n", state->epoch);
 
         // Configura o RTC com o tempo obtido
         struct tm *utc = gmtime(&state->epoch);
         datetime_t t = {
             .year  = utc->tm_year + 1900,
             .month = utc->tm_mon + 1,
             .day   = utc->tm_mday,
             .hour  = utc->tm_hour,
             .min   = utc->tm_min,
             .sec   = utc->tm_sec,
             .dotw  = utc->tm_wday // Dia da semana (0 = domingo)
         };
         rtc_init();
         rtc_set_datetime(&t);
     } else {
         printf("Resposta NTP inválida\n");
     }
     pbuf_free(p);
 }
 
 void ntp_fetch_time(void) {
     NTP_T *state = ntp_init();
     if (!state) return;
 
     cyw43_arch_lwip_begin();
     int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
     cyw43_arch_lwip_end();
 
     state->dns_request_sent = true;
     if (err == ERR_OK) {
         ntp_request(state); // Resultado em cache
     } else if (err != ERR_INPROGRESS) {
         printf("Falha na solicitação DNS\n");
     }
 
     // Aguarda até receber o tempo (timeout simplificado)
     for (int i = 0; i < 50 && state->epoch == 0; i++) {
         cyw43_arch_poll();
         sleep_ms(100);
     }
 
     if (state->epoch == 0) {
         printf("Falha ao obter tempo NTP\n");
     }
 
     udp_remove(state->ntp_pcb);
     free(state);
 }
 
 bool print_rtc_time(struct repeating_timer *t) { // Alterado de void para bool
    datetime_t dt;
    rtc_get_datetime(&dt);

    // Ajusta para BRT (UTC-3)
    int brt_hour = dt.hour - 3;
    if (brt_hour < 0) {
        brt_hour += 24;
        // O ajuste de dia é feito automaticamente pelo RTC ao configurar inicialmente
    }

    // if (estado = RELOGIO) {
    //     printf("Horário atual (BRT): %02d/%02d/%04d %02d:%02d:%02d\n",
    //         dt.day, dt.month, dt.year, brt_hour, dt.min, dt.sec);
    // }
    

    return true; // Retorna true para manter o temporizador ativo
}