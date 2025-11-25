#include "port_scan.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include <string.h>
#include <ctype.h>

static const char *TAG = "PORT_SCANNER";

static void sanitize_banner(char *buffer, int len) {
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = ' ';
        else if (!isprint((int)buffer[i])) buffer[i] = '.'; 
    }
    buffer[len] = '\0'; }

static bool check_tcp(const char *ip, int port, char *banner_out) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) return false;

    struct timeval timeout;
    timeout.tv_sec = CONNECT_TIMEOUT_S;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        close(sock);
        return false; // closed
    }

    // Banner Grabbing
    send(sock, "\r\n", 2, 0); 
    int len = recv(sock, banner_out, MAX_BANNER_LEN - 1, 0);
    
    if (len > 0) {
        sanitize_banner(banner_out, len);
    } else {
        strcpy(banner_out, "(Without Banner)");
    }

    close(sock);
    return true; // open
}

// -1 (Closed), 0 (Open|Filtered), 1 (Open+Data)
static int check_udp(const char *ip, int port, char *banner_out) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) return -1;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = UDP_TIMEOUT_MS * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        close(sock);
        return -1;
    }

    // generic payload
    if (send(sock, "X", 1, 0) < 0) {
        close(sock);
        return -1;
    }

    int len = recv(sock, banner_out, MAX_BANNER_LEN - 1, 0);
    int result = -1;

    if (len > 0) {
        result = 1; // OPEN (Respondeu)
        sanitize_banner(banner_out, len);
    } else {
        strcpy(banner_out, "(Without Banner)");
        if (errno == ECONNREFUSED) {
            result = -1; // CLOSED (ICMP recebido)
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            result = 0; // OPEN|FILTERED (Timeout)
        }
    }
    close(sock);
    return result;
}

int scan_perform_full(const char *target_ip, int start_port, int end_port, scan_result_t *results, int max_results) {
    int count = 0;
    char banner_buffer[MAX_BANNER_LEN];
    
    ESP_LOGI(TAG, "Scan port on %s", target_ip);

    for (int port = start_port; port <= end_port; port++) {
        if (count >= max_results) {
            ESP_LOGW(TAG, "Reached the limit of results");
            break;
        }

        if (check_tcp(target_ip, port, banner_buffer)) {
            // Preenche Struct
            strncpy(results[count].ip_str, target_ip, 16);
            results[count].port = port;
            results[count].protocol = PROTO_TCP;
            results[count].status = STATUS_OPEN;
            strncpy(results[count].banner, banner_buffer, MAX_BANNER_LEN);

            ESP_LOGI(TAG, " IP:Porta: %s:%d | Proto: TCP | Status: OPEN | Banner: [%s]", 
                     target_ip, port, banner_buffer);
            
            count++;
        }

        if (count >= max_results) break;

        int udp_res = check_udp(target_ip, port, banner_buffer);
        if (udp_res >= 0) { // Se for Open ou Open|Filtered
            strncpy(results[count].ip_str, target_ip, 16);
            results[count].port = port;
            results[count].protocol = PROTO_UDP;
            results[count].status = (udp_res == 1) ? STATUS_OPEN : STATUS_OPEN_FILTERED;
            strncpy(results[count].banner, banner_buffer, MAX_BANNER_LEN);

            const char *st_str = (udp_res == 1) ? "OPEN" : "OPEN|FILTERED";
            
            ESP_LOGI(TAG, "IP:Port: %s:%d | Proto: UDP | Status: %s | Banner: [%s]", 
                     target_ip, port, st_str, banner_buffer);

            count++;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Scan finalized. Founded: %d", count);
    return count;
}
