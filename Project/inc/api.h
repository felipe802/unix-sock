#ifndef API_H
#define API_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "http.h"

bool api_handle_request(int client_socket, http_request_t *req);

#endif
