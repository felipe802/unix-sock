# 🌐 **Projeto: Implementação de um Web Server HTTP Concorrente (C/POSIX)**
 Bem-vindo a este repositório de pesquisa e estudo em Engenharia de Sistemas. O foco aqui não é entregar um produto "pronto para produção", mas documentar, explorar e compreender a anatomia de um servidor HTTP concorrente escrito inteiramente em C, interagindo diretamente com as APIs do sistema operacional. Este é um espaço dedicado à investigação técnica sobre como as redes e os sistemas realmente operam por baixo dos panos.

## 🧠 **A Filosofia e o Desafio Arquitetural**
 No ecossistema de desenvolvimento moderno, a comunicação de rede é frequentemente ofuscada por pesadas abstrações (frameworks web, reverse proxies, runtimes gerenciados). O objetivo primário deste projeto não é competir com o Nginx ou Apache, mas sim **desconstruir a abstração**.

 Desenvolver um servidor HTTP/1.1 em **C puro** exige confrontar diretamente a dura realidade da engenharia de sistemas: a rede é não-confiável, a memória é finita e a concorrência gera condições de corrida. Este projeto é um estudo rigoroso sobre:

 1. **O Modelo OSI na Prática:** A transição do fluxo de bytes brutos da Camada 4 (TCP) para o protocolo semântico da Camada 7 (HTTP), lidando com fragmentação de pacotes, reconstrução de *streams* e latência.
 2. **Gerenciamento de Estado sem Garbage Collector:** Como alocar, rastrear e liberar memória (buffers de requisição e resposta) em um ambiente de alta concorrência sem introduzir *Memory Leaks* ou *Use-After-Free*.
 3. **A Evolução da Concorrência:** O estudo propõe uma jornada arquitetural clara. A base teórica começa com a compreensão das regras universais do UNIX (modelos POSIX tradicionais) para, em seguida, explorar a quebra dessas regras em busca de performance extrema utilizando APIs assíncronas e específicas de Kernel (como Event-Driven/kqueue).

---

# 😈 **O Ambiente de Desenvolvimento: Por que FreeBSD?**
 Embora os conceitos teóricos abordados sejam amplamente multiplataforma, o **FreeBSD** foi escolhido como o ecossistema primário de pesquisa, arquitetura e validação. Para engenharia de software de baixo nível (Systems Programming), o FreeBSD oferece vantagens estruturais e ferramentas analíticas que superam alternativas tradicionais.

## 📜 **A Origem Histórica: O Berço dos Sockets**
 O **BSD (Berkeley Software Distribution)** foi o laboratório onde a pilha TCP/IP moderna foi forjada. A **API de Sockets**, adotada universalmente hoje, foi introduzida no **4.2BSD** em 1983. Desenvolver sobre o FreeBSD é trabalhar na implementação "de referência" das redes UNIX.

 > **Referência Oficial:** [FreeBSD Developers Handbook - Sockets Programming](https://docs.freebsd.org/en/books/developers-handbook/sockets/)

## 🏗️ **O "Base System" Coeso**
 Diferente do Linux, que é apenas um Kernel combinado com utilitários GNU de terceiros, o FreeBSD desenvolve o Kernel, a biblioteca padrão C (`libc`) e as ferramentas de *User Space* como um **único repositório coeso (Base System)**. Isso significa que o comportamento das *syscalls*, a implementação da memória e a documentação estão sempre em perfeita sincronia, eliminando a ambiguidade comum ao se debugar problemas profundos de integração.

## 🔬 **Observabilidade Absoluta com DTrace**
 Para construir servidores de alta performance, "achismo" não funciona. O FreeBSD integra nativamente o **DTrace (Dynamic Tracing Framework)**. Ele permite instrumentar e rastrear o servidor em tempo real e em produção, mapeando exatamente quantos milissegundos o Kernel gasta alocando buffers de rede (mbufs), realizando trocas de contexto (*context switches*) no `fork()`, ou travando em operações de I/O de disco, sem precisar alterar uma linha de código C ou recompilar o servidor.

## 🛡️ **Paradigmas Superiores de Arquitetura**
 O FreeBSD expõe primitivas de Kernel consideradas o estado da arte para escalabilidade e segurança de rede:

 * **kqueue vs epoll:** O `kqueue` do FreeBSD não monitora apenas Sockets de rede, mas unifica o monitoramento de processos (`SIGCHLD`), timers, I/O assíncrono e eventos de sistema de arquivos (vnodes) em uma única API elegante.
 * **Segurança Ofensiva/Defensiva (Capsicum):** Enquanto containers dependem de namespaces complexos, o FreeBSD permite que um servidor drope seus próprios privilégios e entre em um "Capability Mode" (`Capsicum`). Se um atacante explorar um *Buffer Overflow* na função de *parsing* HTTP deste servidor, o *Capsicum* bloqueará fisicamente no Kernel qualquer tentativa de abrir novos arquivos ou sockets maliciosos.

---

# 📖 **Recursos e Documentação Oficial**
 Para garantir a integridade do desenvolvimento, utilizamos a documentação oficial do FreeBSD como **Single Source of Truth (SSoT)**.

## 🔎 **Consulta Online (Web)**
 *Melhor para busca indexada e navegação rápida entre capítulos.*

 📚 **Hub Central:** **[FreeBSD Books](https://docs.freebsd.org/en/books/)** — Acesse a biblioteca completa de livros e artigos técnicos.

 | Recurso | Tipo | Descrição | Link |
 | :--- | :--- | :--- | :--- |
 | **FreeBSD Handbook** | Guia | O "guia definitivo" para instalação e administração. | [🌐 **Acessar**](https://docs.freebsd.org/en/books/handbook/) |
 | **Developers Handbook** | Guia | Essencial para **programação de sockets**, chamadas de sistema e arquitetura do kernel. | [🌐 **Acessar**](https://docs.freebsd.org/en/books/developers-handbook/) |
 | **FreeBSD FAQ** | FAQ | Respostas para as dúvidas mais comuns sobre o SO. | [🌐 **Acessar**](https://docs.freebsd.org/en/books/faq/) |
 | **Manual Pages** | Manual | Referência direta de comandos e funções da biblioteca C. | [🌐 **Acessar**](https://man.freebsd.org/) |

## 📥 **Download Offline (PDF)**
 *Ideal para ambientes isolados (air-gapped) ou leitura focada sem distrações.*

 🗂️ **Diretório Completo:** **[FreeBSD Books Archive](https://download.freebsd.org/doc/en/books/)** — Repositório raiz com todos os arquivos para download.

 | Recurso | Download PDF | Download Página |
 | :--- | :--- | :--- |
 | **FreeBSD Handbook** | [📄 **Visualizar PDF**](https://download.freebsd.org/doc/en/books/handbook/handbook_en.pdf) | [📥 **Baixar Página**](https://download.freebsd.org/doc/en/books/handbook/handbook_en.tar.gz) |
 | **Developers Handbook** | [📄 **Visualizar PDF**](https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.pdf) | [📥 **Baixar Página**](https://download.freebsd.org/doc/en/books/developers-handbook/developers-handbook_en.tar.gz) |
 | **FreeBSD FAQ** | [📄 **Visualizar PDF**](https://download.freebsd.org/doc/en/books/faq/faq_en.pdf) | [📥 **Baixar Página**](https://download.freebsd.org/doc/en/books/faq/faq_en.tar.gz) |

---

# 🗺️ **Mapa do Repositório: Teoria e Prática**
 Este repositório é dividido em três pilares fundamentais: o ambiente, a pesquisa teórica e a implementação nua e crua.

## 📚 **A Fundação e o Ecossistema ([`/FreeBSD`](./FreeBSD))**
 O laboratório de testes e a fonte da verdade.
 * **[`/Books`](./FreeBSD/Books/):** Cópias locais em PDF da documentação oficial do FreeBSD (Handbook, Architecture, Developers).
 * **[`/Scripts`](./FreeBSD/Scripts/):** Ferramentas para preparação, limpeza e testes do ambiente de estudo (`setup.sh`, `connect.sh`, etc).

## 🧠 **O Acervo de Pesquisa Arquitetural ([`/Learn`](./Learn))**
 Antes de escrever código, é preciso entender a física e a filosofia das redes. Este diretório contém nossos ensaios e a engenharia de conhecimento do projeto.
 * **[`/Prompts`](./Learn/Prompts/):** O coração da nossa pesquisa. Contém matrizes de "Engenharia de Prompts" rigorosas e inflexíveis, criando personas (como *Arqueólogo de Software* e *Engenheiro de Data Path*) para extrair o conhecimento mais denso possível sobre sistemas e redes.
 * **[`/Markdown`](./Learn/Markdown/):** Os ensaios técnicos exaustivos gerados. Tratados profundos descendo até o nível do silício sobre o Modelo OSI, Zero-Copy (Bypass de Kernel), TCP/UDP, endereçamento L2/L3 e a anatomia dos Kernels UNIX.
 * **[`/Summary`](./Learn/Summary/):** Sínteses e esquemas para revisão rápida de conceitos densos.

## 💻 **A Implementação Bare-Metal ([`/Project`](./Project))**
 Onde a teoria encontra a dura realidade dos ponteiros, *file descriptors* e alocação de memória em C puro.
 * **[`/src`](./Project/src) e [`/inc`](./Project/inc):** O núcleo do Web Server. Implementação bruta do *parser* HTTP, rotinas de rede, gerenciamento do *event loop* de sockets e a lógica da API.
 * **[`/web`](./Project/web/):** O front-end estático (HTML, CSS, JS) contendo a interface de testes que é nativamente servida e gerenciada pelo próprio servidor C.
 * **[`Makefile`](./Project/Makefile):** As diretivas de compilação sem abstrações.

---

# 🛠️ **Acesso Rápido: Arquivos do Repositório**
 Além dos links oficiais, este repositório contém cópias locais da documentação e scripts de automação para facilitar o desenvolvimento no ambiente FreeBSD e Linux.

## 📚 **Livros (PDF Offline)**
 Estes arquivos estão localizados na pasta [`./FreeBSD/Books/`](./FreeBSD/Books/).

 | 📄 Documento | 🔗 Link Local | 📝 Descrição |
 | --- | --- | --- |
 | **FreeBSD Handbook** | [`FreeBSD Handbook.pdf`](./FreeBSD/Books/FreeBSD%20Handbook.pdf) | O guia definitivo de instalação, administração e uso geral do sistema. |
 | **FreeBSD Developers' Handbook** | [`FreeBSD Developers' Handbook.pdf`](./FreeBSD/Books/FreeBSD%20Developers%27%20Handbook.pdf) | Guia avançado focado em programação de Kernel, Sockets e IPC. |
 | **FreeBSD Architecture Handbook** | [`FreeBSD Architecture Handbook.pdf`](./FreeBSD/Books/FreeBSD%20Architecture%20Handbook.pdf) | Detalhes profundos sobre a estrutura e os subsistemas do kernel. |
 | **Design and Implementation of 4.4BSD** | [`The Design and Implementation of the 4.4BSD Operating System.pdf`](./FreeBSD/Books/The%20Design%20and%20Implementation%20of%20the%204.4BSD%20Operating%20System.pdf) | Livro clássico sobre a base de design do 4.4BSD (origem do FreeBSD). |
 | **FreeBSD Porter’s Handbook** | [`FreeBSD Porter’s Handbook.pdf`](./FreeBSD/Books/FreeBSD%20Porter%E2%80%99s%20Handbook.pdf) | Guia oficial para criar "ports" e empacotar softwares de terceiros. |
 | **FreeBSD FAQ** | [`Frequently Asked Questions for FreeBSD.pdf`](./FreeBSD/Books/Frequently%20Asked%20Questions%20for%20FreeBSD.pdf) | Perguntas frequentes e soluções rápidas de problemas comuns. |
 | **FreeBSD Documentation Primer** | [`FreeBSD Documentation Project Primer for New Contributors.pdf`](./FreeBSD/Books/FreeBSD%20Documentation%20Project%20Primer%20for%20New%20Contributors.pdf) | Manual para novos contribuidores do projeto de documentação oficial. |
 | **FreeBSD Project Model** | [`A project model for the FreeBSD Project.pdf`](./FreeBSD/Books/A%20project%20model%20for%20the%20FreeBSD%20Project.pdf) | Estudo sobre a estrutura organizacional e governança do projeto. |
 | **FreeBSD Accessibility Handbook** | [`FreeBSD Accessibility Handbook.pdf`](./FreeBSD/Books/FreeBSD%20Accessibility%20Handbook.pdf) | Guia de utilização e configuração de recursos de acessibilidade. |

## ⚙️ **Scripts de Configuração**
 Scripts utilitários localizados na pasta [`./FreeBSD/Scripts/`](./FreeBSD/Scripts/) para auxiliar na preparação do ambiente.

 * **[`install.sh`](./FreeBSD/Scripts/install.sh)**: Script para a preparação das ferramentas essenciais de análise e configuração do ambiente.
 * **[`setup.sh`](./FreeBSD/Scripts/setup.sh)**: Script para configuração inicial do ambiente (variáveis e permissões).
 * **[`connect.sh`](./FreeBSD/Scripts/connect.sh)**: Script para testes de conectividade e sockets.
 * **[`download.sh`](./FreeBSD/Scripts/download.sh)**: Script para baixar recursos adicionais ou PDFs atualizados.
 * **[`uninstall.sh`](./FreeBSD/Scripts/uninstall.sh)**: Script para limpeza e reset do ambiente de estudo.
