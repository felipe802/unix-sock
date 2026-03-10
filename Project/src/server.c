#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "server.h"

static void log_server_addresses(int port)
{
    struct ifaddrs *interfaces;
    char ip_str[INET_ADDRSTRLEN];

    printf("[INFO]: Server started.\n");
    printf("[INFO]: You can access the server clicking on the links below:\n");

    if (getifaddrs(&interfaces) != 0)
    {
        printf("        -> http://127.0.0.1:%d/\n", port);
        printf("\n[INFO]: Press Ctrl+C or Ctrl+Z to gracefully stop the server.\n\n");
        return;
    }

    for (struct ifaddrs *temp_addr = interfaces; temp_addr != NULL; temp_addr = temp_addr->ifa_next)
    {
        if (temp_addr->ifa_addr != NULL && temp_addr->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *sock_addr = (struct sockaddr_in *)temp_addr->ifa_addr;
            inet_ntop(AF_INET, &sock_addr->sin_addr, ip_str, INET_ADDRSTRLEN);

            printf("        -> http://%s:%d/\n", ip_str, port);
        }
    }

    freeifaddrs(interfaces);
    printf("\n[INFO]: Press Ctrl+C or Ctrl+Z to gracefully stop the server.\n\n");
}

int server_init(int port)
{
    int server_socket;
    struct sockaddr_in socket_address = {};
    int socket_opt = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("[ERR]: Error creating socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)))
    {
        perror("[ERR]: Error in setsockopt");
        exit(EXIT_FAILURE);
    }

    printf("\n[INFO]: Socket created: %d\n", server_socket);

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("[ERR]: Error in bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("[INFO]: Bind successful!\n");

    if (listen(server_socket, 255) < 0)
    {
        perror("[ERR]: Error in listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    log_server_addresses(port);

    return server_socket;
}
