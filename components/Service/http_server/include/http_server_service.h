#ifndef HTTP_SERVER_SERVICE_H
#define HTTP_SERVER_SERVICE_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t start_web_server(void);
esp_err_t stop_web_server(void);
esp_err_t http_server_register_uri(const httpd_uri_t *uri_handler);

#endif // !HTTP_SERVER_SERVICE_H
