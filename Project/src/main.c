#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

#include "main.h"
#include "server.h"
#include "http.h"

static int sig_pipe[2];

static void generic_signal_handler(int signum)
{
    int saved_errno = errno;
    (void)!write(sig_pipe[1], &signum, sizeof(int));
    errno = saved_errno;
}

static int setup_self_pipe(void)
{
    if (pipe(sig_pipe) == -1)
    {
        perror("[ERR]: pipe");
        return -1;
    }

    int flags = fcntl(sig_pipe[0], F_GETFL, 0);
    fcntl(sig_pipe[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(sig_pipe[1], F_GETFL, 0);
    fcntl(sig_pipe[1], F_SETFL, flags | O_NONBLOCK);

    struct sigaction sa = {0};
    sa.sa_handler = generic_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTSTP, &sa, NULL) == -1 ||
        sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("[ERR]: sigaction");
        return -1;
    }

    return sig_pipe[0];
}

static void process_signals(int pipe_read_fd, int *running)
{
    int signum;
    while (read(pipe_read_fd, &signum, sizeof(int)) == sizeof(int))
    {
        if (signum == SIGCHLD)
        {
            int status;
            pid_t pid;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
            {
                printf("[INFO]: Zombie reaped (PID: %d)\n", pid);
            }
        }
        else if (signum == SIGINT || signum == SIGTSTP)
        {
            printf("\n[INFO]: Shutdown signal received...\n");
            *running = 0;
        }
    }
}

static void accept_and_fork(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket < 0)
    {
        perror("[ERR]: accept");
        return;
    }

    printf("[INFO]: New socket created: %d\n", client_socket);

    switch (fork())
    {
    case -1:
        close(client_socket);
        perror("[ERR]: fork");
        break;
    case 0:
        close(server_fd);
        close(sig_pipe[0]);
        close(sig_pipe[1]);
        http_handle_client(client_socket);
        close(client_socket);
        _exit(EXIT_SUCCESS);
    default:
        close(client_socket);
        break;
    }
}

int main()
{
    int pipe_read_fd = setup_self_pipe();
    if (pipe_read_fd == -1)
        return EXIT_FAILURE;

    int server_fd = server_init(PORT);

    struct pollfd fds[2] = {
        {.fd = server_fd, .events = POLLIN, .revents = 0},
        {.fd = pipe_read_fd, .events = POLLIN, .revents = 0}};

    int running = 1;

    while (running)
    {
        int poll_result = poll(fds, 2, -1);

        if (poll_result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("[ERR]: Erro fatal no poll");
            break;
        }

        if (fds[1].revents & POLLIN)
        {
            process_signals(pipe_read_fd, &running);
        }

        if (fds[0].revents & POLLIN)
        {
            accept_and_fork(server_fd);
        }
    }

    printf("[INFO]: Closing server socket and exiting...\n");
    close(server_fd);
    close(sig_pipe[0]);
    close(sig_pipe[1]);
    return EXIT_SUCCESS;
}
