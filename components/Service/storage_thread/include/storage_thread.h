#ifndef STORAGE_THREAD_H
#define STORAGE_THREAD_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Handle global para a fila, para que a API possa acessá-la
extern QueueHandle_t storage_queue;

/**
 * @brief Cria a fila e inicia o thread de armazenamento.
 * Deve ser chamado uma vez no boot do sistema.
 */
void storage_thread_init(void);

#endif // STORAGE_THREAD_H