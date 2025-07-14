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
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "Darth Maul",
            .password = "MyPassword123",
            .ssid_len = strlen("Darth Maul"),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 4,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(84)); 

    ESP_LOGI(TAG, "Wi-Fi inicializado no modo APSTA");
    led_blink_green();
}

void wifi_service_init(void) {
    ESP_LOGI(TAG, "Serviço Wi-Fi inicializado");
}