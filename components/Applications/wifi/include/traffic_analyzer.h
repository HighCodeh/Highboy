#ifndef TRAFFIC_ANALYZER_H
#define TRAFFIC_ANALYZER_H

/**
 * @brief Inicia e exibe a interface do analisador de tráfego de canais Wi-Fi.
 * * Esta função assume o controlo do ecrã, mostrando um gráfico em tempo real
 * dos pacotes por segundo num canal selecionável. A função só retorna
 * quando o utilizador pressiona o botão 'BACK'.
 */
void show_traffic_analyzer(void);

#endif // TRAFFIC_ANALYZER_H