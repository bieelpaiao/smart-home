/**
 * ntp_utils.h
 * Declarações para o cliente NTP.
 */

 #ifndef NTP_UTILS_H
 #define NTP_UTILS_H

 #include <time.h>
 
 #include "pico/stdlib.h"
 #include "pico/cyw43_arch.h"
 #include "lwip/dns.h"
 #include "lwip/pbuf.h"
 #include "lwip/udp.h"
 
 #define NTP_SERVER "pool.ntp.org"
 #define NTP_MSG_LEN 48
 #define NTP_PORT 123
 #define NTP_DELTA 2208988800 // segundos entre 1 Jan 1900 e 1 Jan 1970
 #define NTP_TEST_TIME (30 * 1000) // Sincronizar a cada 30 segundos
 #define NTP_RESEND_TIME (10 * 1000)
 
 // Estrutura para o estado NTP
 typedef struct NTP_T_ {
     ip_addr_t ntp_server_address;
     bool dns_request_sent;
     struct udp_pcb *ntp_pcb;
     absolute_time_t ntp_test_time;
     alarm_id_t ntp_resend_alarm;
     time_t last_epoch; // Última hora sincronizada
     absolute_time_t last_sync_time; // Momento da última sincronização
 } NTP_T;
 
 // Declarações das funções
 void print_time(NTP_T* state);
 void ntp_result(NTP_T* state, int status, time_t *result);
 void ntp_request(NTP_T *state);
 int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
 void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
 void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
 NTP_T* ntp_init(void);
 bool repeating_timer_callback(struct repeating_timer *t);
 void run_ntp_test(void);
 
 #endif // NTP_UTILS_H