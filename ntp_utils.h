/**
 * ntp_utils.h
 * Declarações para o cliente NTP simplificado e RTC.
 */

 #ifndef NTP_UTILS_H
 #define NTP_UTILS_H
 
 #include "pico/stdlib.h"
 #include "pico/cyw43_arch.h"
 #include "lwip/dns.h"
 #include "lwip/pbuf.h"
 #include "lwip/udp.h"
 #include "hardware/rtc.h"
 
 #define NTP_SERVER "pool.ntp.org"
 #define NTP_MSG_LEN 48
 #define NTP_PORT 123
 #define NTP_DELTA 2208988800 // Segundos entre 1 Jan 1900 e 1 Jan 1970
 
 typedef struct NTP_T_ {
     ip_addr_t ntp_server_address;
     bool dns_request_sent;
     struct udp_pcb *ntp_pcb;
     time_t epoch; // Armazena o tempo obtido do NTP
 } NTP_T;
 
 // Declarações das funções
 NTP_T* ntp_init(void);
 void ntp_request(NTP_T *state);
 void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
 void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
 void ntp_fetch_time(void);
 bool print_rtc_time(struct repeating_timer *t); // Alterado de void para bool
 
 #endif // NTP_UTILS_H