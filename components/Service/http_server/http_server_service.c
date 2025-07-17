#include "http_server_service.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "HTTP_SERVICE";
static httpd_handle_t server = NULL;

esp_err_t start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Serviço HTTP iniciado.");
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Falha ao iniciar serviço HTTP.");
    return ESP_FAIL;
}

esp_err_t http_server_register_uri(const httpd_uri_t *uri_handler) {
    if (server) {
        ESP_LOGI(TAG, "Serviço HTTP registrando URI: %s", uri_handler->uri);
        return httpd_register_uri_handler(server, uri_handler);
    }
    ESP_LOGE(TAG, "Servidor não iniciado, impossível registrar URI '%s'", uri_handler->uri);
    return ESP_FAIL;
}

esp_err_t stop_http_server(void) {
    if (server == NULL) {
        ESP_LOGW(TAG, "Servidor HTTP já está parado ou não foi iniciado.");
        return ESP_OK; // Retorna ESP_OK, pois o servidor já está parado
    }

    esp_err_t err = httpd_stop(server);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Servidor HTTP parado com sucesso.");
        server = NULL; // Limpa a handle do servidor após parar
    } else {
        ESP_LOGE(TAG, "Falha ao parar o servidor HTTP: %s", esp_err_to_name(err));
    }

    return err;
}
