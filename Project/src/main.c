#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "main.h"
#include "server.h"
#include "http.h"

static int server_socket_fd = -1;

static void sigchld_handler(int s) {
    (void)s;
    int saved_errno = errno;
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[INFO]: Zombie reaped (PID: %d) | Exit Status: %d\n", pid, WEXITSTATUS(status));
    }

    errno = saved_errno;
}

static void sigint_handler(int s) {
    (void)s;
    const char msg[] = "\n[INFO]: Signal received. Shutting down server gracefully...\n";
    (void)!write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    
    if (server_socket_fd != -1) {
        close(server_socket_fd);
    }
    
    _exit(EXIT_SUCCESS);
}

static void signal_setup() {
    struct sigaction sa_chld = {};
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("[ERR]: Error configuring SIGCHLD");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_int = {};
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("[ERR]: Error configuring SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTSTP, &sa_int, NULL) == -1) {
        perror("[ERR]: Error configuring SIGTSTP");
        exit(EXIT_FAILURE);
    }
}

int main() {
    signal_setup();

    server_socket_fd = server_init(PORT);
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while ((client_socket = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_len))) {
        if (client_socket < 0) {
            if (errno == EINTR || errno == EBADF) break;
            perror("[ERR]: Error in accept");
            continue;
        }
        
        printf("[INFO]: New socket created: %d\n", client_socket);

        switch (fork()) {
        case -1:
            close(client_socket);
            perror("[ERR]: Error creating child process (fork)");
            break;
        case 0:
            close(server_socket_fd);
            printf("[INFO]: Child process (PID: %d) started handling socket %d.\n", getpid(), client_socket);
            http_handle_client(client_socket);
            close(client_socket);
            printf("[INFO]: Child process (PID: %d) closed socket %d and is exiting.\n", getpid(), client_socket);
            _exit(EXIT_SUCCESS);
        default:
            close(client_socket);
            break;
        }
    }

    if (server_socket_fd != -1) close(server_socket_fd);
    printf("[INFO]: Server terminated successfully!\n");
    return EXIT_SUCCESS;
}
