#include "ui_sd.h"
#include "menu.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "home.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "menu_battery.h"
#include "icons.h"
#include "sub_menu.h"
#include "music_menu.h"
#include "ir_menu.h"
#include "pin_def.h"

// ========== VARIÁVEIS ==========
static int menuAtual = 0;
static int menuOffset = 0;

// ========== ÍCONES E TEXTOS ==========
static const uint16_t* icons[] = {
    wifi_main,
    blu_main,
    nfc_main,
    infra_main,
    rf_main,
    sd_main,
    NULL // Placeholder for "Musicas" which might not have an icon
};

static const char* menuTexts[] = {
    "WiFi",
    "Bluetooth",
    "NFC",
    "Infrared",
    "RF",
    "SD",
    "Musicas"
};

static const int menuSize = sizeof(menuTexts) / sizeof(menuTexts[0]);


// ========== FUNÇÕES AUXILIARES ==========
static void executeMenuItem(int index); // Forward declaration

// Interpolação linear (adicionada para evitar erro de linkagem)
static int lerp(int start, int end, float t) {
    return start + (end - start) * t;
}

// ✨ ATUALIZADO: Desenha um item do menu no framebuffer
static void drawMenuItem(int menuIndex, int posY, bool isSelected) {
    const int itemWidth = 220;
    const int itemHeight = 45;
    const int iconSize = 24;
    const int itemX = 10;

    // Cores baseadas na seleção
    uint16_t rect_color = isSelected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_PURPLE;
    uint16_t text_color = isSelected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_WHITE;

    // Posições X e Y
    int textX = itemX + 50;
    int iconX = itemX + 20;
    int iconY = posY + (itemHeight - iconSize) / 2;

    // Desenha o retângulo arredondado (apenas a borda)
    st7789_draw_round_rect_fb(itemX, posY, itemWidth, itemHeight, 8, rect_color);
    
    // Desenha o texto com fundo transparente (cor do texto = cor do fundo)
    st7789_draw_text_fb(textX, posY + 15, menuTexts[menuIndex], text_color, ST7789_COLOR_BLACK);

    // Desenha o ícone
    if (icons[menuIndex] != NULL) {
        // ✨ CORRIGIDO: Ordem dos argumentos corrigida
        st7789_draw_image_fb(iconX, iconY, iconSize, iconSize, icons[menuIndex]);
    }
}

// ✨ ATUALIZADO: Desenha a barra de rolagem no framebuffer
static void drawScrollBar(void) {
    int screenHeight = ST7789_HEIGHT;
    int usableHeight = screenHeight - SCROLL_BAR_TOP;
    int maxVisibleItems = 4;

    // Desenhar pontilhado lateral
    int spacing = 5;
    for (int y = SCROLL_BAR_TOP; y < screenHeight; y += spacing) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_GRAY);
    }

    // Barra de progresso
    float scrollBarHeight = (float)usableHeight * ((float)maxVisibleItems / menuSize);
    if (scrollBarHeight < 6) scrollBarHeight = 6; // altura mínima

    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)menuOffset / (menuSize - maxVisibleItems)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_fill_rect_fb(
        ST7789_WIDTH - SCROLL_BAR_WIDTH,
        SCROLL_BAR_TOP + (int)scrollY,
        SCROLL_BAR_WIDTH,
        (int)scrollBarHeight,
        ST7789_COLOR_WHITE
    );
}

// ========== FUNÇÕES PRINCIPAIS ==========

// Inicializa Menu
void menu_init(void) {
    // A configuração dos GPIOs pode ser feita uma vez na task principal
}

// ✨ ATUALIZADO: Lógica de desenho totalmente refeita para usar framebuffer
void showMenu(void) {
    st7789_set_text_size(2);

    const int maxVisibleItems = 4;
    const int startY = 10;
    const int itemHeight = 60;

    // --- INÍCIO DO FRAME ---
    
    // 1. Limpa o framebuffer inteiro
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    // 2. Ajusta o offset de rolagem se necessário
    if (menuAtual < menuOffset) {
        menuOffset = menuAtual;
    } else if (menuAtual >= menuOffset + maxVisibleItems) {
        menuOffset = menuAtual - maxVisibleItems + 1;
    }

    // 3. Redesenha todos os itens visíveis no framebuffer
    for (int i = 0; i < maxVisibleItems; i++) {
        int menuIndex = i + menuOffset;
        if (menuIndex < menuSize) {
            drawMenuItem(menuIndex, startY + i * itemHeight, menuIndex == menuAtual);
        }
    }

    // 4. Desenha a barra de rolagem no framebuffer
    drawScrollBar();

    // 5. Envia o framebuffer finalizado para a tela de uma só vez
    st7789_flush();
    
    // --- FIM DO FRAME ---
}

// Controle
void handleMenuControls(void) {
    bool stayInMenu = true;
    while (stayInMenu) {
        if (!gpio_get_level(BTN_UP)) {
            menuAtual = (menuAtual - 1 + menuSize) % menuSize;
            showMenu(); // Redesenha a tela
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_DOWN)) {
            menuAtual = (menuAtual + 1) % menuSize;
            showMenu(); // Redesenha a tela
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_OK)) {
            executeMenuItem(menuAtual);
            vTaskDelay(pdMS_TO_TICKS(200));
            // Após sair de um submenu, a tela principal do menu será redesenhada
            showMenu();
        }
        else if (!gpio_get_level(BTN_BACK)) {
            stayInMenu = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Executa item
static void executeMenuItem(int index) {
    switch (index) {
        case 0:
            battery_info_screen();
            break;
        case 1:
            show_wifi_menu();
            break;
        case 2:
            // setupNFCMenu();
            break;
        case 3:
            show_lg_menu();
            break;
        case 4:
            // setupRFMenu();
            break;
        case 5:
            sd_menu_screen();
            break;
        case 6:
            show_music_menu();
            break;
        default:
            break;
    }
}


void menu_task(void *pvParameters) {
    // Configura os GPIOs uma única vez aqui
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_OK) | (1ULL << BTN_BACK) | (1ULL << BTN_LEFT),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    // Estado inicial
    current_state = STATE_HOME;

    while (1) {
        switch (current_state) {
            case STATE_HOME:
                home(); // A função home() também deve usar _fb e terminar com flush()
                while (current_state == STATE_HOME) {
                    if (!gpio_get_level(BTN_LEFT)) {
                        current_state = STATE_MENU;
                        vTaskDelay(pdMS_TO_TICKS(200));
                        break; // Sai do loop interno para a máquina de estados reavaliar
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                break;

            case STATE_MENU:
                showMenu(); // Mostra o estado inicial do menu
                handleMenuControls(); // Entra no loop de controle do menu
                current_state = STATE_HOME; // Ao sair do menu, volta para home
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

