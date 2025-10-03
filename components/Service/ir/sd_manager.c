#include "sd_manager.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Definições dos pinos
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   22

// Configurações - CORRIGIDO: usando /sdcard que funciona
#define MOUNT_POINT "/sdcard"
#define SPI_DMA_CHAN SDSPI_DEFAULT_DMA

static const char *TAG = "SD_MANAGER";
static bool is_initialized = false;
static sdmmc_card_t *card = NULL;

bool sd_manager_init(void) {
    
    if (is_initialized) {
        ESP_LOGW(TAG, "SD Manager já foi inicializado");
        return true;
    }

    esp_err_t ret;

    ESP_LOGI(TAG, "Inicializando barramento SPI...");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar bus SPI: %s", esp_err_to_name(ret));
        return false;
    }

    // Configuração do slot SPI
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    // Configuração do sistema de arquivos - SIMPLIFICADA
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,  // Não formata automaticamente
        .max_files = 5,                   // Reduzido para melhor compatibilidade
        .allocation_unit_size = 16 * 1024 // Cluster size que funciona
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    // Monta o sistema de arquivos
    ESP_LOGI(TAG, "Montando SD card...");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar o sistema de arquivos: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return false;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "SD Card inicializado com sucesso");

    // Imprime informações do cartão
    if (card) {
        sdmmc_card_print_info(stdout, card);
    }

    return true;
}

bool sd_manager_save_file(const char* filename, const char* content) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD Manager não foi inicializado");
        return false;
    }

    if (!filename || !content) {
        ESP_LOGE(TAG, "Parâmetros inválidos");
        return false;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Salvando arquivo: %s", filepath);

    // Abre arquivo para escrita de texto
    FILE* f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para escrita: %s (errno: %d - %s)", 
                 filepath, errno, strerror(errno));
        return false;
    }

    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, f);
    
    // Force sync para garantir escrita no SD
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    if (written != content_len) {
        ESP_LOGE(TAG, "Falha ao escrever dados completos no arquivo");
        return false;
    }

    ESP_LOGI(TAG, "Arquivo salvo com sucesso: %s (%zu bytes)", filename, written);
    return true;
}

// NOVA FUNÇÃO: Salva dados binários (como no seu código que funciona)
bool sd_manager_save_binary(const char* filename, const void* data, size_t size) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD Manager não foi inicializado");
        return false;
    }

    if (!filename || !data || size == 0) {
        ESP_LOGE(TAG, "Parâmetros inválidos");
        return false;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Salvando arquivo binário: %s", filepath);

    // Abre arquivo para escrita binária
    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para escrita: %s (errno: %d - %s)", 
                 filepath, errno, strerror(errno));
        return false;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);

    if (written != size) {
        ESP_LOGE(TAG, "Falha ao escrever dados completos no arquivo");
        return false;
    }

    ESP_LOGI(TAG, "Arquivo binário salvo com sucesso: %s (%zu bytes)", filename, written);
    return true;
}

char* sd_manager_read_file(const char* filename) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD Manager não foi inicializado");
        return NULL;
    }

    if (!filename) {
        ESP_LOGE(TAG, "Nome do arquivo inválido");
        return NULL;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    ESP_LOGI(TAG, "Lendo arquivo: %s", filepath);

    FILE* f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para leitura: %s", filepath);
        return NULL;
    }

    // Determina o tamanho do arquivo
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
        ESP_LOGE(TAG, "Falha ao determinar tamanho do arquivo");
        fclose(f);
        return NULL;
    }

    // Aloca memória para o conteúdo (+1 para \0)
    char* content = malloc(file_size + 1);
    if (content == NULL) {
        ESP_LOGE(TAG, "Falha ao alocar memória para leitura do arquivo");
        fclose(f);
        return NULL;
    }

    // Lê o conteúdo
    size_t read_size = fread(content, 1, file_size, f);
    fclose(f);

    if (read_size != file_size) {
        ESP_LOGE(TAG, "Falha ao ler dados completos do arquivo");
        free(content);
        return NULL;
    }

    // Adiciona terminador nulo
    content[file_size] = '\0';

    ESP_LOGI(TAG, "Arquivo lido com sucesso: %s (%ld bytes)", filename, file_size);
    return content;
}

void sd_manager_deinit(void) {
    if (!is_initialized) {
        ESP_LOGW(TAG, "SD Manager não foi inicializado");
        return;
    }

    // Desmonta o sistema de arquivos
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    
    // Libera o bus SPI
    spi_bus_free(SPI2_HOST);

    is_initialized = false;
    card = NULL;

    ESP_LOGI(TAG, "SD Manager finalizado");
}

bool sd_manager_test(void) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD Manager não inicializado - execute sd_manager_init() primeiro");
        return false;
    }

    ESP_LOGI(TAG, "=== TESTE DE DIAGNÓSTICO DO SD CARD ===");
    
    // Teste 1: Informações do cartão
    if (card) {
        ESP_LOGI(TAG, "✅ Cartão detectado:");
        ESP_LOGI(TAG, "   Nome: %s", card->cid.name);
        ESP_LOGI(TAG, "   Velocidade: %d MHz", card->csd.tr_speed / 1000000);
        ESP_LOGI(TAG, "   Capacidade: %llu MB", ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    } else {
        ESP_LOGE(TAG, "❌ Nenhum cartão detectado");
        return false;
    }

    // Teste 2: Verificar ponto de montagem
    struct stat st;
    if (stat(MOUNT_POINT, &st) == 0) {
        ESP_LOGI(TAG, "✅ Ponto de montagem acessível: %s", MOUNT_POINT);
    } else {
        ESP_LOGE(TAG, "❌ Ponto de montagem inacessível: %s", MOUNT_POINT);
        return false;
    }

    // Teste 3: Criar arquivo de teste
    const char* test_filename = "test_write.tmp";
    const char* test_content = "Teste de escrita SD Card";
    
    if (sd_manager_save_file(test_filename, test_content)) {
        ESP_LOGI(TAG, "✅ Teste de escrita passou");
        
        // Teste 4: Ler arquivo de teste
        char* read_content = sd_manager_read_file(test_filename);
        if (read_content) {
            if (strcmp(read_content, test_content) == 0) {
                ESP_LOGI(TAG, "✅ Teste de leitura passou");
            } else {
                ESP_LOGE(TAG, "❌ Conteúdo lido difere do escrito");
                free(read_content);
                return false;
            }
            free(read_content);
        } else {
            ESP_LOGE(TAG, "❌ Teste de leitura falhou");
            return false;
        }

        // Remove arquivo de teste
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, test_filename);
        if (unlink(filepath) == 0) {
            ESP_LOGI(TAG, "✅ Arquivo de teste removido");
        } else {
            ESP_LOGW(TAG, "⚠️ Falha ao remover arquivo de teste");
        }
    } else {
        ESP_LOGE(TAG, "❌ Teste de escrita falhou");
        return false;
    }

    // Teste 5: Teste binário
    uint32_t test_data[] = {0x12345678, 0xABCDEF00, 0x87654321};
    if (sd_manager_save_binary("test_binary.bin", test_data, sizeof(test_data))) {
        ESP_LOGI(TAG, "✅ Teste de escrita binária passou");
    } else {
        ESP_LOGW(TAG, "⚠️ Teste de escrita binária falhou");
    }

    ESP_LOGI(TAG, "=== DIAGNÓSTICO CONCLUÍDO COM SUCESSO ===");
    return true;
}