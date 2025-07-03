# 📡 Firmware HighBoy (Beta)

Este repositório contém um **firmware em desenvolvimento** para a plataforma **HighBoy**.  
**Atenção:** este firmware está em **fase beta** e **ainda está incompleto**.

---

## Alvos Oficialmente Suportados 
| ESP32-S3 |
| -------- |

---

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

## 🚀 Como utilizar este projeto

Recomendamos que este projeto sirva como base para projetos personalizados com ESP32-S3.  
Para começar um novo projeto com ESP-IDF, siga o guia oficial:  
🔗 [Documentação ESP-IDF - Criar novo projeto](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)

---

## 📁 Estrutura inicial do projeto

Apesar da estrutura modular, o projeto ainda mantém uma organização compatível com o sistema de build do ESP-IDF (CMake).

Exemplo de layout:
```
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

---

# 📡 HighBoy Firmware (Beta)

This repository contains a **firmware in development** for the **HighBoy** platform.
**Warning:** this firmware is in its **beta phase** and is **still incomplete**.

---

## Officially Supported Targets

| ESP32-S3 |
| -------- |

---

## Firmware Structure

Unlike basic examples with a single `main.c`, this project uses a modular structure organized into **components**, which are divided as follows:

- **Drives** – Handles hardware drivers and interfaces.
- **Services** – Implements support functionalities and auxiliary logic.
- **Core** – Contains the system's central logic and main managers.
- **Applications** – Specific applications that use the previous modules.

This division facilitates scalability, code reuse, and firmware organization.

📷 See the general project architecture:
[![Firmware Architecture](pics/arquitetura.png)](pics/arquitetura.png)

---

##  How to use this project

We recommend that this project serves as a basis for custom projects with ESP32-S3.
To start a new project with ESP-IDF, follow the official guide:
🔗 [ESP-IDF Documentation - Create a new project](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)

---

## Initial project structure

Despite the modular structure, the project still maintains an organization compatible with the ESP-IDF build system (CMake).

Example layout:
```
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

- The project is in its **beta** phase, subject to frequent changes.
- Contributions and feedback are welcome to evolve the project.
