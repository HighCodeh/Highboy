#ifndef BAD_USB_H
#define BAD_USB_H

/**
 * @brief Inicializa o driver USB, configura o dispositivo como um teclado (HID)
 * e cria uma tarefa para executar o payload do BadUSB assim que o
 * dispositivo for conectado a um computador.
 *
 * Esta é a única função que precisa ser chamada a partir do app_main.
 */
void bad_usb_init(void);

void bad_usb_stop(void);
/**
 * @brief Executa o payload "Hello World": abre o Bloco de Notas e digita uma mensagem.
 */
void run_payload_helloworld(void);

/**
 * @brief Executa o payload "PowerShell": abre um terminal PowerShell no Windows.
 */
void run_payload_powershell(void);

/**
 * @brief Executa o payload "Rick Roll": abre o navegador no vídeo.
 */
void run_payload_rickroll(void);

void run_payload_linux_rickroll(void);

#endif // BAD_USB_H
