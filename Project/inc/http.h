#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdint.h>

constexpr int HTTP_STATUS_OK = 200;
constexpr int HTTP_STATUS_UNAUTHORIZED = 401;
constexpr int HTTP_STATUS_FORBIDDEN = 403;
constexpr int HTTP_STATUS_NOT_FOUND = 404;
constexpr int HTTP_STATUS_NOT_ALLOWED = 405;

constexpr size_t MAX_HTTP_HEADERS = 32;

typedef enum http_method
{
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_PATCH,
	HTTP_DELETE,
	HTTP_UNKNOWN
} http_method_t;

typedef struct http_header
{
	char *key;
	char *value;
} http_header_t;

typedef struct http_request
{
	http_method_t method;
	char *path;
	char *version;
	size_t header_count;
	http_header_t headers[MAX_HTTP_HEADERS];
	size_t body_len;
	char *body;
} http_request_t;

typedef struct http_response
{
	int status_code;
	const char *status_message;
	const char *content_type;
	size_t header_count;
	http_header_t headers[MAX_HTTP_HEADERS];
	size_t body_len;
	const char *body;
	const char *file_path;
} http_response_t;

void http_send_response(int client_socket, http_response_t *res);
void http_send_error(
    int client_socket, int status_code, const char *status_msg, const char *body
);
void http_handle_client(int client_socket);

#endif
