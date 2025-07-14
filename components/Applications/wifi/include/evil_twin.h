#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

/**
 * @brief Inicia a lógica de backend do ataque Evil Twin.
 *
 * Configura o Access Point com o SSID fornecido, inicia o servidor web
 * e registra os handlers do portal cativo. Esta função é não-bloqueante.
 *
 * @param ssid O nome da rede (SSID) a ser clonada.
 */
void evil_twin_start_attack(const char* ssid);

/**
 * @brief Para a lógica de backend do ataque Evil Twin.
 *
 * Para o servidor web e restaura o estado padrão do Wi-Fi.
 */
void evil_twin_stop_attack(void);

#endif // EVIL_TWIN_H
