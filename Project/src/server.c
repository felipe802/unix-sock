#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"

static void log_server_addresses(uint16_t port, ip_mode_t mode)
{
	struct ifaddrs *interfaces;
	char ip_str[INET6_ADDRSTRLEN];

	printf("[INFO]: Servidor inicializado.\n");
	printf("[INFO]: Acesse o servidor através dos links abaixo:\n");
	printf("        [LocalHost] -> http://localhost:%u/\n", port);

	if (getifaddrs(&interfaces) != 0)
	{
		if (mode == IP_MODE_IPV4_ONLY || mode == IP_MODE_DUAL_STACK)
			printf("        [IPv4]      -> http://127.0.0.1:%u/\n", port);

		if (mode == IP_MODE_IPV6_ONLY || mode == IP_MODE_DUAL_STACK)
			printf("        [IPv6]      -> http://[::1]:%u/\n", port);

		printf("\n[INFO]: Pressione Ctrl+C para encerrar o servidor.\n\n");
		return;
	}

	for (struct ifaddrs *temp_addr = interfaces; temp_addr != nullptr;
	     temp_addr = temp_addr->ifa_next)
	{
		if (temp_addr->ifa_addr == nullptr)
			continue;

		int family = temp_addr->ifa_addr->sa_family;

		if (family == AF_INET && (mode == IP_MODE_IPV4_ONLY || mode == IP_MODE_DUAL_STACK))
		{
			struct sockaddr_in *sock_addr = (struct sockaddr_in *)temp_addr->ifa_addr;
			inet_ntop(AF_INET, &sock_addr->sin_addr, ip_str, sizeof(ip_str));
			printf("        [IPv4]      -> http://%s:%u/\n", ip_str, port);
		}
		else if (
		    family == AF_INET6 && (mode == IP_MODE_IPV6_ONLY || mode == IP_MODE_DUAL_STACK)
		)
		{
			struct sockaddr_in6 *sock_addr6 = (struct sockaddr_in6 *)temp_addr->ifa_addr;
			inet_ntop(AF_INET6, &sock_addr6->sin6_addr, ip_str, sizeof(ip_str));
			printf("        [IPv6]      -> http://[%s]:%u/\n", ip_str, port);
		}
	}

	freeifaddrs(interfaces);
	printf("\n[INFO]: Pressione Ctrl+C para encerrar o servidor.\n\n");
}

static int server_init_ipv4(uint16_t port)
{
	int server_socket;
	struct sockaddr_in socket_address = {};
	int socket_opt = 1;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("[ERRO]: socket IPv4");
		exit(EXIT_FAILURE);
	}

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt));

	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
	{
		perror("[ERRO]: bind IPv4");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	return server_socket;
}

static int server_init_ipv6(uint16_t port, int v6_only)
{
	int server_socket;
	struct sockaddr_in6 socket_address = {};
	int socket_opt = 1;

	server_socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("[ERRO]: socket IPv6");
		exit(EXIT_FAILURE);
	}

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt));

	if (setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, &v6_only, sizeof(v6_only)) < 0)
	{
		perror("[ERRO]: setsockopt IPV6_V6ONLY");
		exit(EXIT_FAILURE);
	}

	socket_address.sin6_family = AF_INET6;
	socket_address.sin6_addr = in6addr_any;
	socket_address.sin6_port = htons(port);

	if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
	{
		perror("[ERRO]: bind IPv6");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	return server_socket;
}

int server_init(uint16_t port, ip_mode_t mode)
{
	int server_socket = -1;

	switch (mode)
	{
	case IP_MODE_IPV4_ONLY:
		printf("\n[INFO]: Configurando socket exclusivo para IPv4...\n");
		server_socket = server_init_ipv4(port);
		break;

	case IP_MODE_IPV6_ONLY:
		printf("\n[INFO]: Configurando socket exclusivo para IPv6...\n");
		server_socket = server_init_ipv6(port, 1);
		break;

	case IP_MODE_DUAL_STACK:
		printf("\n[INFO]: Configurando socket Híbrido (Dual-Stack IPv4/IPv6)...\n");
		server_socket = server_init_ipv6(port, 0);
		break;
	}

	printf("[INFO]: Socket criado: %d | Bind concluído!\n", server_socket);

	if (listen(server_socket, SOMAXCONN) < 0)
	{
		perror("[ERRO]: Error in listen");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	log_server_addresses(port, mode);

	return server_socket;
}
