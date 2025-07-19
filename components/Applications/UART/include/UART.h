#ifndef UART_MONITOR_H
#define UART_MONITOR_H

/**
 * @brief Inicializa o hardware UART necessário para o monitor.
 * * Deve ser chamado uma vez na inicialização do sistema.
 */
void uart_monitor_init(void);

/**
 * @brief Inicia a tela e a lógica do monitor UART.
 * * Esta função é bloqueante e só retornará quando o usuário
 * sair do monitor (ex: pressionando o botão 'Voltar').
 */
void uart_monitor_start(void);

/**
 * @brief Desliga o driver UART e libera os recursos.
 * */
void uart_monitor_deinit(void);

#endif // UART_MONITOR_H