#include "hb_hal_core.h"
#include "hb_hal_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hb_hal_i2c.h"
#include "hb_hal_button.h"
#include "hb_hal_st7789.h"


void app_main(void)
{
    // Inicializa o núcleo do sistema
    if (hb_hal_core_init() != HB_HAL_OK) {
        ESP_LOGE("MAIN", "Falha na inicialização do núcleo");
        return;
    }
    ESP_LOGI("MAIN", "Sistema inicializado com sucesso.");

    i2c_scan();
    hb_button_init(); // Inicializa os botões

    while (1) {
        hb_button_loop(); 

        // Verifica botões
        if (hb_button_is_pressed(HB_BUTTON_OK)) {
            ESP_LOGI("MAIN", "Botão OK pressionado!");
        }

        if (hb_button_is_held(HB_BUTTON_UP)) {
            ESP_LOGI("MAIN", "Botão UP segurado!");
        }

        // Adicione mais verificações conforme necessário
    }

}
