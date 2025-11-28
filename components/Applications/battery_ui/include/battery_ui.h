#ifndef BATTERY_UI_H
#define BATTERY_UI_H

#include <stdint.h> // Para uint16_t, se necessário por outras funções declaradas aqui

// Inclui o cabeçalho do BQ25896 porque a UI faz referência aos tipos dele, como bq25896_charge_status_t
#include "bq25896.h" 

/**
 * @brief Exibe a tela de status da bateria no LCD.
 * * Esta função entra em um loop para ler periodicamente os dados da bateria
 * do driver BQ25896 e atualiza a interface gráfica na tela.
 * A função sai do loop quando o botão 'Voltar' é pressionado.
 */
void show_battery_screen(void);

// Se houver outras funções em battery_ui.c que você deseja expor
// para outros arquivos, declare-as aqui.
// Por exemplo:
// static void draw_battery_ui(int percentage, uint16_t voltage_mv, bq25896_charge_status_t status);
// Não precisamos declarar funções 'static' em .h, pois elas são internas ao .c

#endif // BATTERY_UI_H