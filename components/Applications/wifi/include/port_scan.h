#ifndef SCAN_PORT_H
#define SCAN_PORT_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_BANNER_LEN 64       
#define CONNECT_TIMEOUT_S 1   // seconds
#define UDP_TIMEOUT_MS 500   // miliseconds 

typedef enum {
  PROTO_TCP,
  PROTO_UDP
} scan_protocol_t;

typedef enum {
  STATUS_OPEN,            
  STATUS_OPEN_FILTERED    
} port_status_t;

typedef struct {
  char ip_str[16];                
  int port;                       
  scan_protocol_t protocol;       
  port_status_t status;           
  char banner[MAX_BANNER_LEN];    
} scan_result_t;

int scan_perform_full(const char *target_ip, int start_port, int end_port, scan_result_t *results, int max_results);

#endif
