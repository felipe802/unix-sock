#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdbool.h>

#define HTTP_STATUS_OK 200
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_FORBIDDEN 403
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_NOT_ALLOWED 405

typedef enum
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
    http_header_t headers[32];
    int header_count;
    char *body;
} http_request_t;

typedef struct http_response
{
    int status_code;
    const char *status_message;
    const char *content_type;
    http_header_t headers[16];
    int header_count;
    const char *body;
    size_t body_len;
    const char *file_path;
} http_response_t;

void http_send_response(int client_socket, http_response_t *res);
void http_send_error(int client_socket, int status_code, const char *status_msg, const char *body);
void http_handle_client(int client_socket);

#endif
