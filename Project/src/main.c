#include <stdio.h> // I/O
#include <stdlib.h>     // Tipos de saída
#include <sys/socket.h> // API fundamental de redes
#include <netinet/in.h> // Estruturas para endereços IP
#include <unistd.h> // Chamadas de sistema POSIX
#include <signal.h> // API de interrupções de software (sinais)
#include <sys/wait.h>  // Para a função waitpid 
#include <errno.h>  // Variável de erro global do SO
#include <poll.h>   // Multiplexação de I/O (o vigia)
#include <fcntl.h>  // File control (muda propriedades do tubo)

#include "main.h"
#include "server.h"
#include "http.h"

static int sig_pipe[2]; // Um tubo é apenas um array de 2 números inteiros
// O índice 0 é a boca de leitura, o 1 é a boca de escrita

// O SO interrompe tudo para rodar isso. Deve ser rápido e seguro
static void generic_signal_handler(int signum) {
    // Salvamos o código de erro global (Thread principal) para não atrapalhar alguma
    // outra função que estava rodando antes da interrupção (não corromper o estado)
    int saved_errno = errno;
    
    // Escrevemos os 4 bytes do número do sinal que recebemos dentro do tubo
    // O (void)! é uma diretiva agressiva para ignorar os avisos do compilador sobre
    // o retorno da função write
    // Como colocar um bilhete dizendo que o sinal chegou
    (void)!write(sig_pipe[1], &signum, sizeof(int));
    errno = saved_errno; // Restauramos o estado
}

// Configuração do tubo e sinais
static int setup_self_pipe(void) {
    // pipe() pede ao SO para criar o tubo de comunicação na memória 
    // sig_pipe[0] lê, sig_pipe[1] escreve
    if (pipe(sig_pipe) == -1) {
        perror("[ERR]: pipe");
        return -1;
    }

    // Aqui o tubo é transformado em não-bloqueante
    // 1. Lemos as configurações atuais do arquivo no Kernel (F_GETFL)
    int flags = fcntl(sig_pipe[0], F_GETFL, 0);
    // 2. Reescrevemos as configurações adicionando o bit O_NONBLOCK (F_SETFL)
    fcntl(sig_pipe[0], F_SETFL, flags | O_NONBLOCK);
    // Tornando a boca de escrita (1) não-bloqueante
    flags = fcntl(sig_pipe[1], F_GETFL, 0);
    fcntl(sig_pipe[1], F_SETFL, flags | O_NONBLOCK);

    // Configurando quem vai ouvir os sinais
    struct sigaction sa = {0};
    sa.sa_handler = generic_signal_handler; // Aponta para a nossa função
    sigemptyset(&sa.sa_mask); // Não bloquear outros sinais enquanto este roda

    // Se o sinal interromper uma syscall lenta (como um read() do teclado),
    // diga ao SO para reiniciar essa syscall silenciosamente ao invés de abortá-la
    sa.sa_flags = SA_RESTART;

    // Conectamos nossa função a três gatilhos mortais do SO:
    // SIGINT (Ctrl+C) + SIGSTP (Ctrl+Z) e SIGCHLD (morte de processo filho)
    if (sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTSTP, &sa, NULL) == -1 ||
        sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("[ERR]: sigaction"); return -1;
    }
    // Devolvemos a boca de leitura para a main() vigiar
    return sig_pipe[0];
}

// O processador de eventos do tubo
static void process_signals(int pipe_read_fd, int *running)
{
    int signum;
    // Lemos 4 bytes (um int) por vez da boca de leitura do tubo
    // while drena todo o tubo até esvaziar
    while (read(pipe_read_fd, &signum, sizeof(int)) == sizeof(int))
    {
        // Se um filho que morreu
        if (signum == SIGCHLD) {
            int status;
            pid_t pid;
            // '-1' signfica que deve verificar qualquer filho
            // O 'WNOHANG' significa que não deve travar aqui se não tiver filho morto
            // O while garante que se 10 filhos morreram juntos, os 10 serão limpos
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                // Ao colher o status, o filho morto é oficialmente removido da RAM
                printf("[INFO]: Zombie reaped (PID: %d)\n", pid);
            }
        }
        // Se o admin apertou Ctrl+C ou Ctrl+Z
        else if (signum == SIGINT || signum == SIGTSTP) {
            printf("\n[INFO]: Shutdown signal received...\n");
            // Alteramos o ponteiro que controla o laço principal para 0
            // Isso inicia o "graceful shutdown" do servidor
            *running = 0;
        }
    }
}

// Aceitar clientes e clonar o servidor para atende-los
static void accept_and_fork(int server_fd)
{
    // Estrutura para guardar o IP e a porta de quem está se conectando
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // O accept() vai na fila do kernel (que criamos com o listen no server.c)
    // e puxa o primeiro cliente da fila. Ele cria um novo fd (cliente_socket)
    // exclusivo para conversar com uma pessoa
    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket < 0) {
        perror("[ERR]: accept");
        return;
    }
    printf("[INFO]: New socket created: %d\n", client_socket);

    // fork() faz a clonagem do programa na memória RAM
    // Ele retorna duas vezes, uma no pai e outra no filho
    switch (fork()) {
        case -1: // Faltou memória RAM para criar outro processo
            close(client_socket);
            perror("[ERR]: fork");
            break;
        case 0: // Eis o processo filho
            // O filho é o mensageiro, não atende a porta principal, então a fecha
            close(server_fd);
            // O filho não gerencia o servidor, então ele desliga o rádio de sinais do pai
            close(sig_pipe[0]); close(sig_pipe[1]);
            // Entra na camada de aplicação, lendo e respondendo o HTTP
            http_handle_client(client_socket);
            // Fecha quando termina de enviar o HTML, desliga na cara do cliente
            close(client_socket);
            // _exit com underline morre de imediato sem limpar buffers da biblioteca C
            // sendo a forma mais segura de matar um processo filho sem interferir no pai
            _exit(EXIT_SUCCESS);
        default: // O processo pai
            // O pai nunca conversa com os clientes
            close(client_socket);
            break;
    }
}

int main()
{
    // 1. Prepara a armadilha do tubo para não morrer com sinais do SO
    int pipe_read_fd = setup_self_pipe();
    if (pipe_read_fd == -1) return EXIT_FAILURE;

    // 2. Abre as portas 8080 para a internet (chama nosso server.c)
    int server_fd = server_init(PORT);

    // 3. Monta o array de vigilância para o poll() 
    struct pollfd fds[2] = {
        // fds[0] -> vigia o servidor de rede. POLLIN: avisa se alguem se conectar
        {.fd = server_fd, .events = POLLIN, .revents = 0},
        // fds[1] -> vigia a boca do tubo. POLLIN: avisa se cair um sinal de erro ali
        {.fd = pipe_read_fd, .events = POLLIN, .revents = 0}};

    int running = 1; // Variável de controle (graceful shutdown)

    while (running)
    {
        // O programa dormirá aqui
        // O '2' diz que são 2 portas para vigiar 
        // O '-1' significa Timeout Infinito (dormir para sempre até algo acontecer)
        int poll_result = poll(fds, 2, -1);
        if (poll_result < 0) // Quando ocorre uma interrupção
        {
            // Se o erro foi um sinal do SO (EINTR), não é um erro fatal, apenas volta ao começo do loop
            if (errno == EINTR) continue;
            perror("[ERR]: Erro fatal no poll");
            break;
        }

        // poll() foi acionado. 'revents' (returned events) é preenchido pelo kernel com a resposta
        // O tubo é acionado quando tem sinal lá dentro
        if (fds[1].revents & POLLIN) {
            // Lemos o tubo, enterramos os zumbis ou desligamos a variável 
            // running se foi um Ctrl+C
            process_signals(pipe_read_fd, &running);
        }

        // Se a porta de rede piscou, tem cliente novo
        if (fds[0].revents & POLLIN) {
            // Atende o cliente e faz o fork()
            accept_and_fork(server_fd);
        }
    }

    // Fase de limpeza, só chega aqui quando o admin aperta Ctrl+C
    printf("[INFO]: Closing server socket and exiting...\n");
    // Devolvendo os FD ao kernel
    close(server_fd);
    close(sig_pipe[0]);
    close(sig_pipe[1]);
    return EXIT_SUCCESS;
}
