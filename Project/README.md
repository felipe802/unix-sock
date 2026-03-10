# 💻 **Implementação: Servidor HTTP/1.1 RESTful em C (POSIX / FreeBSD & Linux)**

## 🎯 **Arquitetura e Objetivo Técnico (V1)**
 Implementação de um servidor web e API REST robusta em **C puro (C23)**, operando diretamente sobre a API padrão de **Berkeley Sockets**. Este projeto foi desenhado com foco estrito em **portabilidade UNIX (POSIX)**, garantindo compilação e execução nativa e impecável tanto em ambientes **FreeBSD** quanto **Linux**.

 O objetivo primário é a exploração aprofundada da pilha TCP/IP, protocolos da Camada de Aplicação (HTTP/1.1) e as armadilhas clássicas de I/O e concorrência no nível do SO, adotando o padrão de *Clean Architecture* e *Zero-Copy Parsing*.

### ⚙️ **Modelo de Concorrência: Fork-per-Request & Sincronismo via Self-Pipe Trick**
 Para garantir que múltiplas requisições de rede sejam atendidas simultaneamente sem bloqueio de I/O, o servidor adota a system call UNIX padrão `fork()`, aliada a um sofisticado controle de estado:

 * **Isolamento de Falhas (Workers):** Cada conexão de cliente é delegada a um processo filho isolado em seu próprio espaço de memória, protegendo o daemon principal contra falhas críticas.
 * **O "Self-Pipe Trick" e `poll()`:** Para superar a limitação assíncrona perigosa dos tratadores de sinais POSIX (`sigaction`), o servidor utiliza o lendário *Self-Pipe Trick*. Sinais do Kernel (como `SIGCHLD` e `SIGINT`) são capturados e imediatamente injetados num `pipe` local não-bloqueante. O daemon principal utiliza a chamada `poll()` para multiplexar, monitorando **simultaneamente e de forma síncrona** a chegada de novos clientes (sockets) e a chegada de sinais (pipe).
 * **Resiliência a `EINTR`:** O loop de eventos é desenhado para suportar e ignorar *Interrupted System Calls* geradas por interrupções do Kernel, garantindo uptime ininterrupto.

### ⏱️ **Ciclo de Vida da Conexão e Event Loop**
 O diagrama abaixo ilustra a segregação de responsabilidades entre o Daemon Principal, o Kernel e os Processos Filhos, demonstrando o fluxo síncrono do *Self-Pipe Trick*.

 ```mermaid
 sequenceDiagram
     participant C as HTTP Client
     participant M as Master (poll)
     participant P as Self-Pipe
     participant OS as Kernel UNIX
     participant W as Child Worker

     M->>P: setup pipe() & sigaction()
     loop Main Event Loop
         M->>OS: poll(server_fd, pipe_read_fd)

         alt Novo Cliente HTTP
             C->>OS: TCP 3-Way Handshake
             OS-->>M: POLLIN no server_fd
             M->>W: fork() (Clone de memória)
             Note over W: Worker Isolado
             W->>C: Zero-Copy HTTP Parse & API Roteamento
             W->>C: HTTP/1.1 Resposta (Static ou JSON)
             W->>OS: _exit(0)

         else Sinal Assíncrono (Worker Morreu)
             OS->>M: Kernel emite SIGCHLD
             Note over M: Signal Handler
             M->>P: write(pipe_write_fd, SIGCHLD)
             OS-->>M: POLLIN no pipe_read_fd
             M->>P: read() consome sinal do cano
             M->>OS: waitpid(WNOHANG) -> Zombie Reaped!
         end
     end
 ```

## ⚡ **Definição dos Métodos RESTful Suportados**
 O servidor atua como um back-end *Stateless*, mapeando os verbos HTTP para operações atômicas no diretório `/data/`.

 | Método | Comportamento no Servidor | Finalidade Técnica |
 | --- | --- | --- |
 | **GET** | Leitura via I/O padrão (`fread`) | Recuperação de recursos estáticos ou serialização de diretórios em JSON. |
 | **POST** | Criação via `fopen(..., "w")` | Processamento de buffers e criação integral de novos estados/recursos. |
 | **PUT** | Escrita integral e Idempotente | Substituição completa de um recurso existente numa URI específica. |
 | **PATCH** | Modificação via `fopen(..., "a")` | Atualização parcial, anexando novos dados (*append*) ao final do recurso. |
 | **DELETE** | Remoção via syscall `remove()` | Exclusão definitiva de um recurso de dados no sistema de arquivos. |

## 🔨 **Compilação e Deploy**
 O projeto utiliza um `.editorconfig` para padronização global (LF, UTF-8, Trailing Whitespaces) e é gerenciado via `Makefile`. O compilador **GCC** é exigido com suporte estrito ao padrão C23 (`-std=c23`) e proteções de memória (`-Wall -Wextra`).

 ```sh
 make clear
 make build
 make run
 ```

---

## 🚀 **Roadmap e Evolução Arquitetural (Versão 2)**
 Embora a V1 foque em portabilidade máxima e nos fundamentos POSIX raiz, a arquitetura foi desenhada para permitir uma evolução visando **Alta Performance Absoluta (High Concurrency)**. A Versão 2 abraçará as otimizações nativas de Kernel:

 1. **I/O Assíncrono Direto (O(1)):** Substituição do modelo híbrido `poll`/`fork()` para multiplexação massiva utilizando **`kqueue`/`kevent**` no FreeBSD (ou `epoll` no Linux), eliminando a necessidade do *Self-Pipe Trick*.
 2. **Zero-Copy Transfer:** Evolução do parser atual para a utilização da system call nativa **`sendfile()`**, roteando os bytes estáticos do cache do Kernel diretamente para a NIC (Placa de Rede).
 3. **Multithreading:** Implementação de um *Thread Pool* (pthreads) acoplado ao `kqueue` para processamento escalável em CPUs multicore, abolindo o custo do *context switch* de processos completos.
