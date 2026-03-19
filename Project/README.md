# 💻 **Implementação: Servidor HTTP/1.1 RESTful em C (POSIX / FreeBSD & Linux)**

## 🎯 **Arquitetura e Objetivo Técnico (V1)**
 Implementação de um servidor web e API REST robusta em **C puro (C23)**, operando diretamente sobre a API padrão de **Berkeley Sockets**. Este projeto foi desenhado com foco estrito em **portabilidade UNIX (POSIX)**, garantindo execução nativa tanto em ambientes **FreeBSD** quanto **Linux**.

 O objetivo primário é a exploração da pilha TCP/IP e a superação das armadilhas clássicas de I/O, adotando o padrão de *Clean Architecture* e *In-Place Parsing*.

### ⚙️ **Modelo de Concorrência e Processamento de Rede**
 Para garantir que múltiplas requisições sejam atendidas simultaneamente sem bloqueio de I/O e que o protocolo HTTP seja respeitado rigorosamente, o servidor adota a system call `fork()`, aliada a um sofisticado controle de estado e memória:

 * **Isolamento de Falhas (Workers):** Cada conexão é delegada a um processo filho isolado, protegendo o daemon principal.
 * **O "Self-Pipe Trick" e `poll()`:** Para superar a limitação assíncrona dos sinais POSIX, o servidor utiliza um `pipe` local não-bloqueante. Sinais do Kernel são injetados no cano e monitorados de forma síncrona pelo `poll()` junto com os sockets de rede.
 * **Resiliência a `EINTR`:** O loop de eventos captura e trata *Interrupted System Calls*, garantindo estabilidade durante a limpeza de processos zumbis.
 * **Enquadramento (Framing) TCP Seguro:** O parser HTTP implementa uma máquina de estados resiliente para remontagem de payloads fragmentados na rede, baseando-se dinamicamente no cabeçalho `Content-Length`. Isso imuniza o servidor contra a fragmentação natural da Camada de Transporte.

### ⏱️ **Ciclo de Vida da Conexão (V1)**
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
 			M->>W: fork()
 			W->>C: In-Place HTTP Parsing & Response
 			W->>OS: _exit(0)

 		else Sinal Assíncrono (SIGCHLD)
 			OS->>M: Kernel emite Sinal
 			M->>P: write(pipe_write_fd, signo)
 			OS-->>M: POLLIN no pipe_read_fd
 			M->>OS: waitpid(WNOHANG) -> Zombie Reaped!
 		end
 	end
 ```

## ⚡ **Definição dos Métodos RESTful Suportados**
 | Método | Comportamento no Servidor | Finalidade Técnica |
 | --- | --- | --- |
 | **GET** | Leitura via I/O padrão (`fread`) | Recursos estáticos ou serialização JSON. |
 | **POST** | Criação via `fopen(..., "w")` | Criação integral de novos recursos. |
 | **PUT** | Escrita Idempotente | Substituição completa de um recurso existente. |
 | **PATCH** | Modificação via `fopen(..., "a")` | Atualização parcial do recurso *(Nota: implementado via append de I/O em C, como adaptação ao conceito de modificação parcial).* |
 | **DELETE** | Remoção via syscall `remove()` | Exclusão definitiva no sistema de arquivos. |

## 🔨 **Compilação e Deploy**
 O projeto utiliza um `.editorconfig` para padronização global e é gerenciado via `Makefile`. Requer **GCC** com suporte a **C23**.

 ```sh
 make clear
 make build
 make run
 ```

---

## 🚀 **Roadmap e Evolução Arquitetural (Versão 2)**
 A Versão 2 focará em **Alta Performance Absoluta** e escalabilidade horizontal, migrando do modelo de processos para um modelo de eventos assíncronos.

### **Proposta Técnica V2: Event-Driven Architecture**
 ```mermaid
 graph TD
 	subgraph "Kernel Space"
 		K_NET[Network Interface]
 		K_SIG[Signal Events]
 		K_FS[Filesystem Cache]
 	end

 	subgraph "User Space (V2 Engine)"
 		EP[Event Multiplexer: kqueue / epoll]
 		TQ[Task Queue]
 		
 		subgraph "Thread Pool (Workers)"
 			T1[Thread 1]
 			T2[Thread 2]
 			T3[Thread N]
 		end
 	end

 	K_NET -- "O(1) Readiness" --> EP
 	K_SIG -- "Signal Event" --> EP
 	EP -- "Task Dispatch" --> TQ
 	TQ -- "Pop" --> T1
 	TQ -- "Pop" --> T2

 	T1 -- "syscall: sendfile()" --> K_FS
 	K_FS -- "Direct DMA Transfer" --> K_NET

 	style EP fill:#3b82f6,stroke:#fff,color:#fff
 	style TQ fill:#1e293b,stroke:#fff,color:#fff
 	style K_NET fill:#10b981,stroke:#fff,color:#fff 
 ```

 1. **I/O Assíncrono Nativo:** Substituição do `poll()` por **`kqueue`** (FreeBSD) ou **`epoll`** (Linux), permitindo gerenciar milhares de conexões com complexidade $O(1)$.
 2. **Otimização de Kernel (Zero-Copy):** Implementação da system call **`sendfile()`**, eliminando o *overhead* de copiar bytes do espaço do Kernel para o espaço do usuário antes de enviá-los à rede.
 3. **Thread Pooling:** Transição para um modelo de Threads persistentes, eliminando o custo de criação de processos e reduzindo drasticamente o *Context Switching*.

---

## 🛠️ **Configuração do Ambiente (Setup Environment)**

Para garantir a reprodutibilidade da compilação, o suporte rigoroso ao padrão **C23** e a estabilidade do servidor, as instruções abaixo configuram o ecossistema de desenvolvimento do zero. Você pode optar por configurar a toolchain via Homebrew ou via APT (recomendado para Ubuntu/Debian).

### 🧰 Setup Utils
Instalação das ferramentas de rede e transferência essenciais. O `netcat` é fundamental para testes locais de *raw sockets* e depuração manual do protocolo HTTP, enquanto `curl` e `wget` são necessários para os scripts de automação subsequentes.
 ```bash
 sudo apt install --yes netcat-openbsd
 sudo apt install --yes curl
 sudo apt install --yes wget
 ```

### 📂 Setup Directory
Prepara um diretório local seguro para binários de usuário (`~/.local/bin`) e o injeta na variável de ambiente `$PATH`. Isso evita a poluição do sistema e garante que as versões customizadas (e mais recentes) dos compiladores tenham prioridade de execução sobre as versões nativas do SO.
 ```bash
 mkdir -p "${HOME}/.local/bin"
 cat << 'EOF' | tee -a "${HOME}/.bashrc" > "/dev/null"
 export PATH="${HOME}/.local/bin:$PATH"
 EOF
 ```

### 🍺 Setup Brew (Linuxbrew)
*(Opcional se utilizar apenas o APT)*. Instalação do gerenciador de pacotes Homebrew. É uma alternativa robusta para obter pacotes na modalidade *bleeding-edge* (as versões mais recentes possíveis), operando no espaço de usuário (User Space) sem comprometer as dependências do sistema operacional.
 ```bash
 NONINTERACTIVE=1 bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
 cat << 'EOF' | tee -a "${HOME}/.bashrc" > "/dev/null"
 eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv bash)"
 EOF
 eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"
 ```

### 🧱 Setup GNU Compiler Collection (GCC) — *Via Homebrew*
Baixa a versão mais recente do GCC pelo Homebrew e cria links simbólicos (*symlinks*) dinâmicos no seu diretório `~/.local/bin`. O script resolve a versão mais atual lançada no repositório FTP da GNU e mapeia rigorosamente todas as ferramentas da suíte (como `g++`, `gcc-ar`, `gcc-nm`) para uso imediato pelo processo de `make`.
 ```bash
 brew install gcc
 brew install binutils

 BINUTILS_BIN="$(brew --prefix binutils)/bin"
 for TOOL_PATH in "${BINUTILS_BIN}"/*; do
 	TOOL="$(basename "${TOOL_PATH}")"
 	ln -sf "${TOOL_PATH}" "${HOME}/.local/bin/${TOOL}"
 done

 BREW_BIN="$(brew --prefix)/bin"
 GCC_TARGET="x86_64-pc-linux-gnu"
 GCC_VER="$(ls "${BREW_BIN}" | grep -Eo '^gcc-[0-9]+$' | cut -d- -f2 | sort -n | tail -1)"
 GCC_TOOLS=(
 	"gcc" "g++" "c++" "cpp" "gcov" "gcc-ar" "gcc-nm" "gcc-ranlib"
 	"${GCC_TARGET}-gcc" "${GCC_TARGET}-g++" "${GCC_TARGET}-c++"
 	"${GCC_TARGET}-gcc-ar" "${GCC_TARGET}-gcc-nm" "${GCC_TARGET}-gcc-ranlib"
 	"${GCC_TARGET}-gfortran" "${GCC_TARGET}-gm2"
 )
 for TOOL in "${GCC_TOOLS[@]}"; do
 	ln -sf "${BREW_BIN}/${TOOL}-${GCC_VER}" "${HOME}/.local/bin/${TOOL}"
 done

 ln -sf "${HOME}/.local/bin/gcc" "${HOME}/.local/bin/cc"
 ln -sf "${HOME}/.local/bin/g++" "${HOME}/.local/bin/CC"
 ```

### 🐧 Setup GNU Compiler Collection (GCC) — *Via APT*
*(Alternativa recomendada para estabilidade no Ubuntu)*. Este bloco adiciona o repositório oficial de testes da toolchain (`ubuntu-toolchain-r/test`), garantindo acesso ao suporte do **C23**. O comando `update-alternatives` é orquestrado para forçar o Kernel a adotar esta versão recém-instalada como o padrão absoluto de compilação C/C++ em todo o ambiente.
 ```bash
 sudo add-apt-repository --yes "ppa:ubuntu-toolchain-r/test"
 sudo apt update
 sudo apt install --yes build-essential

 GCC_VER="$(curl -s https://ftp.gnu.org/gnu/gcc/ | grep -o 'href="gcc-[0-9]*' | cut -d- -f2 | sort -n | tail -1)"
 sudo apt install --yes "gcc-$GCC_VER"
 sudo apt install --yes "g++-$GCC_VER"
 sudo update-alternatives --install "/usr/bin/gcc" gcc "/usr/bin/gcc-$GCC_VER" 100 \
 	--slave "/usr/bin/g++" g++ "/usr/bin/g++-$GCC_VER" \
 	--slave "/usr/bin/c++" c++ "/usr/bin/c++-$GCC_VER" \
 	--slave "/usr/bin/cpp" cpp "/usr/bin/cpp-$GCC_VER" \
 	--slave "/usr/bin/gcov" gcov "/usr/bin/gcov-$GCC_VER" \
 	--slave "/usr/bin/gcc-ar" gcc-ar "/usr/bin/gcc-ar-$GCC_VER" \
 	--slave "/usr/bin/gcc-nm" gcc-nm "/usr/bin/gcc-nm-$GCC_VER" \
 	--slave "/usr/bin/gcc-ranlib" gcc-ranlib "/usr/bin/gcc-ranlib-$GCC_VER"
 ```

### 🐉 Setup Clang / LLVM Toolchain
Instalação da arquitetura de compiladores LLVM/Clang. Fortemente recomendada devido às suas excelentes ferramentas de *linting*, *Language Server Protocol* (`clangd`) e sanitização de memória. O script utiliza o instalador dinâmico oficial (`llvm.sh`), extrai a versão estável mais atual da API do GitHub, normaliza as chaves do repositório APT e registra o ecossistema completo no sistema de alternativas.
 ```bash
 sudo apt update
 sudo apt install --yes clang
 sudo apt install --yes libclang-dev

 CLANG_VER="$(curl -s "https://api.github.com/repos/llvm/llvm-project/releases/latest" | grep "tag_name" | cut -d '"' -f 4 | sed 's/llvmorg-//' | cut -d. -f1)"
 wget -qO- "https://apt.llvm.org/llvm.sh" | sudo bash -s -- "$CLANG_VER" all
 sudo grep -l "apt.llvm.org" /etc/apt/sources.list.d/*.list | \
 	xargs sudo sed -i 's/deb http/deb [arch=amd64] http/g'
 sudo update-alternatives --install /usr/bin/clang clang "/usr/bin/clang-$CLANG_VER" 100 \
 	--slave "/usr/bin/clang++" clang++ "/usr/bin/clang++-$CLANG_VER" \
 	--slave "/usr/bin/clangd" clangd "/usr/bin/clangd-$CLANG_VER" \
 	--slave "/usr/bin/clang-format" clang-format "/usr/bin/clang-format-$CLANG_VER" \
 	--slave "/usr/bin/clang-tidy" clang-tidy "/usr/bin/clang-tidy-$CLANG_VER" \
 	--slave "/usr/bin/lldb" lldb "/usr/bin/lldb-$CLANG_VER" \
 	--slave "/usr/bin/llvm-config" llvm-config "/usr/bin/llvm-config-$CLANG_VER" \
 	--slave "/usr/bin/llvm-ar" llvm-ar "/usr/bin/llvm-ar-$CLANG_VER" \
 	--slave "/usr/bin/llvm-nm" llvm-nm "/usr/bin/llvm-nm-$CLANG_VER" \
 	--slave "/usr/bin/llvm-ranlib" llvm-ranlib "/usr/bin/llvm-ranlib-$CLANG_VER"
 sudo update-alternatives --install "/usr/bin/lld" lld "/usr/bin/lld-$CLANG_VER" 100 \
 	--slave "/usr/bin/ld.lld" ld.lld "/usr/bin/ld.lld-$CLANG_VER"
 ```
