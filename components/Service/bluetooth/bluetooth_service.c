#include "bluetooth_service.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "assert.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_stop.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/dis/ble_svc_dis.h"

static const char *TAG = "BLE_SERVICE";

// --- Variáveis Globais Estáticas ---
static uint8_t own_addr_type;
static bool ble_initialized = false;
static SemaphoreHandle_t ble_hs_synced_sem = NULL; // Semáforo para sincronização

// --- Protótipos de Funções Estáticas ---
static void bluetooth_service_on_reset(int reason);
static void bluetooth_service_on_sync(void);
static void bluetooth_service_host_task(void *param);
static int bluetooth_service_gap_event(struct ble_gap_event *event, void *arg);

// --- Implementação das Funções ---

// Callback para eventos GAP
static int bluetooth_service_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Dispositivo conectado! status=%d conn_handle=%d",
                     event->connect.status, event->connect.conn_handle);
            if (event->connect.status != 0) {
                // Falha na conexão, reinicia o anúncio
                bluetooth_service_start_advertising();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Dispositivo desconectado! reason=%d", event->disconnect.reason);
            // Reinicia o anúncio para permitir novas conexões
            bluetooth_service_start_advertising();
            break;
        case BLE_GAP_EVENT_ADV_COMPLETE:
             ESP_LOGI(TAG, "Anúncio concluído!");
             // Reinicia para continuar visível
             bluetooth_service_start_advertising();
             break;
        default:
            break;
    }
    return 0;
}

// Inicia o anúncio BLE
esp_err_t bluetooth_service_start_advertising(void) {
    if (!ble_initialized) {
        ESP_LOGE(TAG, "BLE não inicializado. Não é possível anunciar.");
        return ESP_FAIL;
    }

    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.uuids16 = (ble_uuid16_t[]){ BLE_UUID16_INIT(BLE_SVC_DIS_UUID16) };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    const char* device_name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao configurar campos de anúncio; rc=%d", rc);
        return ESP_FAIL;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, bluetooth_service_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao iniciar anúncio; rc=%d", rc);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Anúncio padrão iniciado com sucesso");
    return ESP_OK;
}

// Para o anúncio BLE
esp_err_t bluetooth_service_stop_advertising(void) {
    if (!ble_initialized) {
        ESP_LOGE(TAG, "BLE não inicializado.");
        return ESP_FAIL;
    }
    if (ble_gap_adv_active()) {
        return ble_gap_adv_stop();
    }
    return ESP_OK;
}

// Callback chamado quando a pilha BLE é resetada
void bluetooth_service_on_reset(int reason) {
    ESP_LOGE(TAG, "Resetando estado; reason=%d", reason);
}

// Callback chamado quando a pilha BLE está sincronizada
void bluetooth_service_on_sync(void) {
    ESP_LOGI(TAG, "BLE sincronizado");
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao inferir tipo de endereço: %d", rc);
        return;
    }
    // Libera o semáforo para sinalizar que a inicialização está completa
    if (ble_hs_synced_sem != NULL) {
        xSemaphoreGive(ble_hs_synced_sem);
    }
}

// Tarefa principal que executa o loop de eventos do NimBLE
void bluetooth_service_host_task(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Iniciada");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Inicializa o serviço Bluetooth completo
esp_err_t bluetooth_service_init(void) {
    if (ble_initialized) {
        ESP_LOGW(TAG, "BLE já inicializado");
        return ESP_OK;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar NimBLE: %s", esp_err_to_name(ret));
        return ret;
    }

    ble_hs_cfg.reset_cb = bluetooth_service_on_reset;
    ble_hs_cfg.sync_cb = bluetooth_service_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ret = ble_svc_gap_device_name_set("ESP32-NimBLE-Svc");
    assert(ret == 0);

    ble_hs_synced_sem = xSemaphoreCreateBinary();
    if (ble_hs_synced_sem == NULL) {
        ESP_LOGE(TAG, "Falha ao criar semáforo");
        return ESP_ERR_NO_MEM;
    }

    nimble_port_freertos_init(bluetooth_service_host_task);

    ESP_LOGI(TAG, "Aguardando sincronização do Host BLE...");
    if (xSemaphoreTake(ble_hs_synced_sem, pdMS_TO_TICKS(10000)) == pdFALSE) {
        ESP_LOGE(TAG, "Timeout na sincronização do Host BLE");
        bluetooth_service_stop();
        return ESP_ERR_TIMEOUT;
    }

    ble_initialized = true;
    ESP_LOGI(TAG, "Inicialização do BLE concluída com sucesso");
    return ESP_OK;
}

// Para o serviço Bluetooth e libera todos os recursos
esp_err_t bluetooth_service_stop(void) {
    if (!ble_initialized) {
        ESP_LOGW(TAG, "BLE não inicializado ou já parado");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Parando serviço BLE");
    int rc = nimble_port_stop();
    if (rc != 0) {
        ESP_LOGE(TAG, "Falha ao parar a porta NimBLE: %d", rc);
    }
    
    nimble_port_deinit();

    if (ble_hs_synced_sem != NULL) {
        vSemaphoreDelete(ble_hs_synced_sem);
        ble_hs_synced_sem = NULL;
    }
    
    ble_initialized = false;
    ESP_LOGI(TAG, "Serviço BLE parado");
    return ESP_OK;
}

// Função para acessar a variável own_addr_type de outros módulos
uint8_t bluetooth_service_get_own_addr_type(void) {
    return own_addr_type;
}

