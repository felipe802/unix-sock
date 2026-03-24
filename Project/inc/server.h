#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>

typedef enum ip_mode
{
	IP_MODE_IPV4_ONLY,
	IP_MODE_IPV6_ONLY,
	IP_MODE_DUAL_STACK
} ip_mode_t;

[[nodiscard]] int server_init(uint16_t port, ip_mode_t mode);

#endif
