#include "wifi_service.h"
#include "led_control.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "virtual_display_client.h" // Adicionar este include

static wifi_ap_record_t stored_aps[WIFI_SCAN_LIST_SIZE];
static uint16_t stored_ap_count = 0;
static SemaphoreHandle_t wifi_mutex = NULL;

static const char *TAG = "wifi_service";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Estação conectada ao AP, MAC: " MACSTR, MAC2STR(event->mac));
        led_blink_green();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Estação desconectada do AP, MAC: " MACSTR, MAC2STR(event->mac));
        led_blink_red();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ESP_LOGI(TAG, "IP atribuído a estação conectada ao AP");
        led_blink_green();
    }
}

void wifi_change_to_hotspot(const char *new_ssid) {
    ESP_LOGI(TAG, "Tentando mudar o SSID do AP para: %s (aberto)", new_ssid);

    // Parar o Wi-Fi para aplicar as novas configurações
    esp_err_t err = esp_wifi_stop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao parar o Wi-Fi: %s", esp_err_to_name(err));
        led_blink_red(); // Opcional: indicar falha
        return;
    }

    // Configurações do novo AP
    wifi_config_t new_ap_config = {
        .ap = {
            // Garante que o SSID não exceda o tamanho máximo e termina com null
            .ssid_len = strlen(new_ssid),
            .channel = 1, // Pode manter o canal ou torná-lo configurável
            .authmode = WIFI_AUTH_OPEN, // Define o AP como aberto (sem senha)
            .max_connection = 4, // Número máximo de conexões
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    
    strncpy((char *)new_ap_config.ap.ssid, new_ssid, sizeof(new_ap_config.ap.ssid) - 1);
    new_ap_config.ap.ssid[sizeof(new_ap_config.ap.ssid) - 1] = '\0'; // Garante null-termination

    // Como a senha é aberta, o campo 'password' pode ser vazio ou não usado,
    // mas por boas práticas, asseguramos que está vazio se não for necessário.
    new_ap_config.ap.password[0] = '\0';

    // Definir as novas configurações
    err = esp_wifi_set_config(WIFI_IF_AP, &new_ap_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao definir a nova configuração do AP Wi-Fi: %s", esp_err_to_name(err));
        led_blink_red(); // Opcional: indicar falha
        return;
    }

    // Iniciar o Wi-Fi com as novas configurações
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar o Wi-Fi com a nova configuração: %s", esp_err_to_name(err));
        led_blink_red(); // Opcional: indicar falha
        return;
    }

    ESP_LOGI(TAG, "SSID do AP Wi-Fi alterado com sucesso para: %s (aberto)", new_ssid);
    led_blink_green(); // Opcional: indicar sucesso
}

void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap(); // Cria a interface AP
    esp_netif_create_default_wifi_sta(); // Cria a interface STA (importante para o display virtual)

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registra o handler de eventos do wifi_service
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_event_handler, NULL, NULL));

    // **** NOVO: Registra o handler de eventos do virtual_display_client para os eventos STA ****
    // ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, virtual_display_wifi_event_handler, NULL, NULL)); 
    // ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, virtual_display_wifi_event_handler, NULL, NULL)); 

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "Darth Maul",
            .password = "MyPassword123",
            .ssid_len = strlen("Darth Maul"),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

    // Configuração do modo STA para o display virtual
    // wifi_config_t sta_config = {
    //     .sta = {
    //         .ssid = WIFI_SSID_VIRTUAL_DISPLAY,     
    //         .password = WIFI_PASS_VIRTUAL_DISPLAY, 
    //     },
    // };
    // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    // Conecta no modo STA para o display virtual
    //esp_wifi_connect(); 

    // NOVO: Inicia a lógica de envio de frames do display virtual
    //virtual_display_start_frame_sending(); 

    ESP_LOGI(TAG, "Inicialização do Wi-Fi em modo APSTA concluída.");
}

void wifi_service_perform_scan(void) {
    if (wifi_mutex == NULL) {
        wifi_mutex = xSemaphoreCreateMutex();
    }

    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Falha ao obter mutex Wi-Fi para scan");
        return;
    }

    // Configuração do scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL, 
        .bssid = NULL, 
        .channel = 0, 
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    ESP_LOGI(TAG, "Iniciando scan de redes (Service)...");
    
    // É boa prática garantir que estamos em um modo que suporte scan
    // O modo APSTA suporta scan, mas o scan acontece na interface STA
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar scan: %s", esp_err_to_name(ret));
        led_blink_red();
        xSemaphoreGive(wifi_mutex);
        return;
    }

    // Obtém os resultados
    uint16_t ap_count = WIFI_SCAN_LIST_SIZE;
    ret = esp_wifi_scan_get_ap_records(&ap_count, stored_aps);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter resultados do scan: %s", esp_err_to_name(ret));
        led_blink_red();
    } else {
        stored_ap_count = ap_count;
        ESP_LOGI(TAG, "Encontrados %d pontos de acesso.", stored_ap_count);
        led_blink_blue();
    }

    xSemaphoreGive(wifi_mutex);
}

uint16_t wifi_service_get_ap_count(void) {
    return stored_ap_count;
}

wifi_ap_record_t* wifi_service_get_ap_record(uint16_t index) {
    if (index < stored_ap_count) {
        return &stored_aps[index];
    }
    return NULL;
}

esp_err_t wifi_service_connect_to_ap(const char *ssid, const char *password) {
    if (ssid == NULL) {
        ESP_LOGE(TAG, "SSID can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Configurating Wi-Fi connection to:: %s", ssid);

    wifi_config_t wifi_config = {0};
    
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    esp_wifi_disconnect(); 

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config STA: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init connection: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Connection request send with success.");
    return ESP_OK;
}

void wifi_service_init(void) {
  wifi_mutex = xSemaphoreCreateMutex();
  ESP_LOGI(TAG, "WIFI service initalized.");
}
