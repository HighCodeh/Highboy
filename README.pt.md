<p align="center">
  <img src="pics/Highboy_repo.png" alt="HighBoy Banner" width="1000"/>
</p>

# 📡 Firmware HighBoy (Beta)

> 🌍 **Languages**:  [🇺🇸 English](README.md) | [🇧🇷 Português](README.pt.md) 

Este repositório contém um **firmware em desenvolvimento** para a plataforma **HighBoy**.  
**Atenção:** este firmware está em **fase beta** e **ainda está incompleto**.




## Alvos Oficialmente Suportados

| ESP32-S3 |
| -------- |




## Estrutura do Firmware

Diferente de exemplos básicos com um único `main.c`, este projeto utiliza uma estrutura modular organizada em **components**, que se dividem da seguinte forma:

- **Drives** – Lida com drivers e interfaces de hardware.  
- **Services** – Implementa funcionalidades de suporte e lógica auxiliar.  
- **Core** – Contém a lógica central do sistema e gerenciadores principais.  
- **Applications** – Aplicações específicas que utilizam os módulos anteriores.

Essa divisão facilita a escalabilidade, reutilização de código e organização do firmware.

📷 Veja a arquitetura geral do projeto:  
[![Arquitetura do Firmware](pics/arquitetura.png)](pics/arquitetura.png)

--- 


## Como utilizar este projeto

Recomendamos que este projeto sirva como base para projetos personalizados com ESP32-S3.  
Para começar um novo projeto com ESP-IDF, siga o guia oficial:  
🔗 [Documentação ESP-IDF - Criar novo projeto](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)


## 📁 Estrutura inicial do projeto

Apesar da estrutura modular, o projeto ainda mantém uma organização compatível com o sistema de build do ESP-IDF (CMake).

Exemplo de layout:

```bash
├── CMakeLists.txt
├── components
│   ├── Drives
│   ├── Services
│   ├── Core
│   └── Applications
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md
```

- O projeto está em fase **beta**, sujeito a mudanças frequentes.
- Contribuições e feedbacks são bem-vindos para evoluir o projeto.




## 🤝 Nossos Apoiadores

Agradecemos especialmente aos parceiros que apoiam este projeto:

[![PCBWay](pics/PCBway.png)](https://www.pcbway.com)
