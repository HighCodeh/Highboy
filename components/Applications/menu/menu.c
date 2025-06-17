#include "ui_sd.h"
#include "menu.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "menu_battery.h"
#include "icons.h"
#include "sub_menu.h"
#include "music_menu.h"
#include "ir_menu.h"

// ========== DEFINIÇÕES ==========
#define BTN_UP     15
#define BTN_DOWN   6
#define BTN_OK     4
#define BTN_BACK   7
#define BTN_LEFT 5  

#define SCROLL_BAR_WIDTH 4
#define SCROLL_BAR_TOP 0

// ========== PROTÓTIPOS ==========
static int lerp(int start, int end, float t);
static void drawMenuItem(int menuIndex, int posY, bool isSelected);
static void drawScrollBar(void);
static void executeMenuItem(int index);

// ========== VARIÁVEIS ==========
static int menuAtual = 0;
static int menuOffset = 0;
static int lastMenuAtual = -1;
static bool fundoDesenhado = false;

// ========== ÍCONES E TEXTOS ==========
static const uint16_t* icons[] = {
    wifi_main,
    blu_main,
    nfc_main,
    infra_main,
    rf_main,
   sd_main,
   NULL
};

static const char* menuTexts[] = {
    "WiFi",
    "Bluetooth",
    "NFC",
    "Infrared",
    "RF",
    "SD",
    "MUsicas"
};

static const int menuSize = sizeof(menuTexts) / sizeof(menuTexts[0]);


// ========== FUNÇÕES AUXILIARES ========== // [demais funções seguem abaixo sem alterações] 



// Interpolação linear
static int lerp(int start, int end, float t) {
    return start + (end - start) * t;
}

// Desenha 1 item
static void drawMenuItem(int menuIndex, int posY, bool isSelected) {
    const int itemWidth = 220;
    const int itemHeight = 45;
    const int iconSize = 24;
    int textX = isSelected ? lerp(50, 65, 1.0) : 50;
    int iconX = isSelected ? lerp(20, 35, 1.0) : 20;
    int iconY = posY + (itemHeight - iconSize) / 2;

    st7789_draw_rect(10, posY, itemWidth, itemHeight, ST7789_COLOR_BLACK);
    st7789_draw_round_rect(10, posY, itemWidth, itemHeight, 8, isSelected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_PURPLE);
    st7789_draw_text(textX, posY + 15, menuTexts[menuIndex], isSelected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_WHITE);

    if (icons[menuIndex] != NULL) {
        st7789_draw_bitmapRGB(iconX, iconY, icons[menuIndex], iconSize, iconSize);  // ✅ CERTO

    }
}

// Barra de Scroll estilo Flipper
static void drawScrollBar(void) {
    int screenHeight = ST7789_HEIGHT;
    int usableHeight = screenHeight - SCROLL_BAR_TOP;
    int maxVisibleItems = 4;

    // Limpar a área da barra de rolagem
    st7789_draw_rect(ST7789_WIDTH - SCROLL_BAR_WIDTH, SCROLL_BAR_TOP, SCROLL_BAR_WIDTH, usableHeight, ST7789_COLOR_BLACK);

    // Desenhar pontilhado lateral
    int spacing = 5;
    for (int y = SCROLL_BAR_TOP; y < screenHeight; y += spacing) {
        st7789_draw_pixel(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_GRAY);
    }

    // Barra de progresso
    float scrollBarHeight = (float)usableHeight * ((float)maxVisibleItems / menuSize);
    if (scrollBarHeight < 6) scrollBarHeight = 6; // altura mínima

    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)menuOffset / (menuSize - maxVisibleItems)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_draw_rect(
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
    fundoDesenhado = false;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_OK) | (1ULL << BTN_BACK),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
}

void showMenu(void) {

    

    st7789_set_text_size(2);

    const int maxVisibleItems = 4;
    const int startY = 10;
    const int itemHeight = 60;
    const int itemWidth = 220;

    bool needFullRedraw = false;

    static int lastMenuOffset = -1;



    if (!fundoDesenhado) {
        st7789_fill_screen(ST7789_COLOR_BLACK);
        fundoDesenhado = true;
        needFullRedraw = true;
    }

    // Detecta se houve scroll (mudança no menuOffset)
    if (menuAtual < menuOffset) {
        menuOffset = menuAtual;
    } else if (menuAtual >= menuOffset + maxVisibleItems) {
        menuOffset = menuAtual - maxVisibleItems + 1;
    }

    if (menuOffset != lastMenuOffset) {
        needFullRedraw = true;

        // Limpar área dos itens antes de redesenhar
        for (int i = 0; i < maxVisibleItems; i++) {
            int y = startY + i * itemHeight;
            st7789_draw_rect(10, y, itemWidth, itemHeight, ST7789_COLOR_BLACK);
        }
    }

    if (needFullRedraw) {
        // Redesenhar todos os itens visíveis
        for (int i = 0; i < maxVisibleItems; i++) {
            int menuIndex = i + menuOffset;
            if (menuIndex < menuSize) {
                drawMenuItem(menuIndex, startY + i * itemHeight, menuIndex == menuAtual);
            }
        }
    } else {
        // Não scrollou: redesenha apenas o item alterado
        if (menuAtual != lastMenuAtual) {
            int oldY = startY + (lastMenuAtual - menuOffset) * itemHeight;
            int newY = startY + (menuAtual - menuOffset) * itemHeight;

            if (lastMenuAtual >= menuOffset && lastMenuAtual < menuOffset + maxVisibleItems) {
                drawMenuItem(lastMenuAtual, oldY, false); // Item antigo
            }
            if (menuAtual >= menuOffset && menuAtual < menuOffset + maxVisibleItems) {
                drawMenuItem(menuAtual, newY, true); // Novo selecionado
            }
        }
    }

    drawScrollBar(); // Atualiza a barra de rolagem

    lastMenuAtual = menuAtual;
    lastMenuOffset = menuOffset;
}



// Controle
void handleMenuControls(void) {
    bool stayInMenu = true;
    while (stayInMenu) {
        if (!gpio_get_level(BTN_UP)) {
            menuAtual = (menuAtual - 1 + menuSize) % menuSize;
            showMenu();
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_DOWN)) {
            menuAtual = (menuAtual + 1) % menuSize;
            showMenu();
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_OK)) {
            fundoDesenhado = false;
            executeMenuItem(menuAtual);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (!gpio_get_level(BTN_BACK)) {
            fundoDesenhado = false;
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
        show_wifi_menu(); // <- ADICIONE ESTA LINHA
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
            sd_menu_screen();  // chama a tela SD
            break;
            case 6:
            show_music_menu();
             break;
        default:
            break;
    }
} 