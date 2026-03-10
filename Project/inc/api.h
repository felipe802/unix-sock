#ifndef API_H
#define API_H

#include <stdbool.h>
#include "http.h"

bool api_handle_request(int client_socket, http_request_t *req);

#endif
