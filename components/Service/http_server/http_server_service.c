#include "http_server_service.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "sd_card_read.h"
#include <stdbool.h>

static const char *TAG = "HTTP_SERVICE";
static const char *TAG_SD = "HTTP_SDCARD";

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

esp_err_t http_service_register_uri(const httpd_uri_t *uri_handler) {
  if (server) {
    ESP_LOGI(TAG, "Serviço HTTP registrando URI: %s", uri_handler->uri);
    return httpd_register_uri_handler(server, uri_handler);
  }
  ESP_LOGE(TAG, "Servidor não iniciado, impossível registrar URI '%s'",
           uri_handler->uri);
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

// use this function with sd_card lib
const char *get_html_buffer(const char *path) {
    size_t file_size = 0; 
    
    esp_err_t err = sd_read_get_file_size(path, &file_size);

    if (err != ESP_OK) {
        ESP_LOGE(TAG_SD, "Error to obtain file size: %s (%s)", path, esp_err_to_name(err));
        return NULL;
    }

    if (file_size == 0) {
        ESP_LOGE(TAG_SD, "Empty file: %s", path);
        return NULL;
    }

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        ESP_LOGE(TAG_SD, "Alocation failed (OOM)");
        return NULL;
    }

    if (sd_read_string(path, buffer, file_size + 1) != ESP_OK) {
        free(buffer);
        return NULL;
    }

    ESP_LOGI(TAG_SD, "Success Buffer Retrieved: %zu bytes", file_size);
    return buffer;
}

static httpd_err_code_t map_http_status_to_esp_err(int code) {
  switch (code) {
  case 400:
    return HTTPD_400_BAD_REQUEST;
  case 401:
    return HTTPD_401_UNAUTHORIZED;
  case 403:
    return HTTPD_403_FORBIDDEN;
  case 404:
    return HTTPD_404_NOT_FOUND;
  case 408:
    return HTTPD_408_REQ_TIMEOUT;
  case 500:
    return HTTPD_500_INTERNAL_SERVER_ERROR;

  default:
    return HTTPD_500_INTERNAL_SERVER_ERROR;
  }
}

// creating a "castimg", its a function to do the same thing the original. 
// Because i dont want to import both http_service(that im created) and esp_http_server(esp_idf) in the same place, this is repetitive
// this seenms wrong to me, so here is the functions to make everything more beautiful and make just one include, boilerplate for a good cause:D  
esp_err_t http_service_send_error(httpd_req_t *req, http_status_t status_code, const char *msg) {

  httpd_err_code_t error_code = map_http_status_to_esp_err(status_code);

  return httpd_resp_send_err(req, error_code, msg);
}

esp_err_t  http_service_req_recv(httpd_req_t *req, char *buffer, size_t buffer_size){
  // note: content_len comes with header HTTP. 
  if (req->content_len >= buffer_size) {
      ESP_LOGE(TAG, "Content length (%d) bigger than buffer (%d)", req->content_len, buffer_size);
      http_service_send_error(req, HTTP_STATUS_BAD_REQUEST_400, "Request content too long");
      return ESP_ERR_INVALID_SIZE;
  }

  int ret = httpd_req_recv(req, buffer, req->content_len);

  if (ret <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
          http_service_send_error(req, HTTP_STATUS_REQUEST_TIMEOUT_408, "Timeout receiving data");
      }
      return ESP_FAIL;
  }

  buffer[ret] = '\0';

  return ESP_OK;
}

esp_err_t http_service_query_key_value(const char *data_buffer, const char *key, char *out_val, size_t out_size) {
    esp_err_t err = httpd_query_key_value(data_buffer, key, out_val, out_size);

    if (err == ESP_OK) {
        return ESP_OK;
    } 
    else if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGD(TAG, "Key '%s' not found in buffer", key);
    } 
    else if (err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        ESP_LOGE(TAG, "Key value '%s' get locked (small out buffer)", key);
    }
    
    return err;
}



esp_err_t http_service_send_response(httpd_req_t *req, const char *buffer, ssize_t length) {
    if (buffer == NULL) {
        ESP_LOGW(TAG, "Attemp to sent NULL buffer. Sending empty response.");
        // return httpd_resp_send(req, NULL, 0);
        return http_service_send_error(req, HTTP_STATUS_INTERNAL_ERROR_500, "Internal Server Error: No content generated");
    }

    // Envia a resposta
    esp_err_t err = httpd_resp_send(req, buffer, length);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error to sent HTTP response: %s", esp_err_to_name(err));
        // Aqui você poderia adicionar lógica extra, como fechar sockets forçadamente se necessário
    } else {
        ESP_LOGD(TAG, "Successfully send response (%d bytes)", length);
    }

    return err;
}
