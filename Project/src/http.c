#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "api.h"

#define STATIC_DIR "./web/"
#define RECV_BUFFER_SIZE 8192
#define PATH_BUFFER_SIZE 1024
#define HEADER_BUFFER_SIZE 1024
#define FILE_CHUNK_SIZE 8192

static void ensure_static_directory()
{
	struct stat st = {};
	if (stat(STATIC_DIR, &st) == -1)
		mkdir(STATIC_DIR, 0755);
}

static const char *get_mime_type(const char *path)
{
	const char *ext = strrchr(path, '.');
	if (!ext)
		return "application/octet-stream";

	static const struct
	{
		const char *ext;
		const char *type;
	} mime_map[] = {
		{".html", "text/html"},
		{".css", "text/css"},
		{".js", "application/javascript"},
		{".json", "application/json"},
		{".png", "image/png"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"}};

	size_t map_size = sizeof(mime_map) / sizeof(mime_map[0]);
	for (size_t i = 0; i < map_size; i++)
	{
		if (strcmp(ext, mime_map[i].ext) == 0)
			return mime_map[i].type;
	}

	return "application/octet-stream";
}

static http_method_t parse_method(const char *method_str)
{
	static const struct
	{
		const char *str;
		http_method_t method;
	} method_map[] = {
		{"GET", HTTP_GET},
		{"POST", HTTP_POST},
		{"PUT", HTTP_PUT},
		{"PATCH", HTTP_PATCH},
		{"DELETE", HTTP_DELETE}};

	size_t map_size = sizeof(method_map) / sizeof(method_map[0]);
	for (size_t i = 0; i < map_size; i++)
	{
		if (strcmp(method_str, method_map[i].str) == 0)
			return method_map[i].method;
	}
	return HTTP_UNKNOWN;
}

static void parse_http_request(char *buffer, http_request_t *req)
{
	req->header_count = 0;
	req->body = NULL;
	req->body_len = 0;

	char *saveptr;
	char *line = strtok_r(buffer, "\r\n", &saveptr);
	if (!line)
		return;

	char *method_str = line;
	char *path_str = strchr(method_str, ' ');
	if (path_str)
	{
		*path_str = '\0';
		path_str++;
		while (*path_str == ' ')
			path_str++;

		char *version_str = strchr(path_str, ' ');
		if (version_str)
		{
			*version_str = '\0';
			version_str++;
			while (*version_str == ' ')
				version_str++;
			req->version = version_str;
		}

		char *query = strchr(path_str, '?');
		if (query)
		{
			*query = '\0';
		}

		req->path = path_str;
	}

	req->method = parse_method(method_str);

	while ((line = strtok_r(NULL, "\r\n", &saveptr)) && req->header_count < MAX_HTTP_HEADERS)
	{
		char *colon = strchr(line, ':');
		if (colon)
		{
			*colon = '\0';
			req->headers[req->header_count].key = line;
			req->headers[req->header_count].value = colon + 1;
			while (*(req->headers[req->header_count].value) == ' ')
				req->headers[req->header_count].value++;
			req->header_count++;
		}
	}
}

void http_send_response(int client_socket, http_response_t *res)
{
	char header_buffer[HEADER_BUFFER_SIZE];
	size_t offset = 0;
	int ret;

	ret = snprintf(header_buffer + offset, sizeof(header_buffer) - offset,
				   "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nConnection: close\r\n",
				   res->status_code, res->status_message, res->content_type);
	if (ret > 0)
		offset += (size_t)ret;

	for (size_t i = 0; i < res->header_count && i < MAX_HTTP_HEADERS; i++)
	{
		ret = snprintf(header_buffer + offset, sizeof(header_buffer) - offset,
					   "%s: %s\r\n", res->headers[i].key, res->headers[i].value);
		if (ret > 0)
			offset += (size_t)ret;
	}

	if (res->file_path)
	{
		FILE *file = fopen(res->file_path, "rb");
		if (!file)
			return;

		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		ret = snprintf(header_buffer + offset, sizeof(header_buffer) - offset,
					   "Content-Length: %ld\r\n\r\n", file_size);
		if (ret > 0)
			offset += (size_t)ret;

		send(client_socket, header_buffer, offset, 0);

		char chunk[FILE_CHUNK_SIZE];
		size_t bytes_read;
		while ((bytes_read = fread(chunk, 1, sizeof(chunk), file)) > 0)
		{
			if (send(client_socket, chunk, bytes_read, 0) < 0)
				break;
		}
		fclose(file);
	}
	else
	{
		ret = snprintf(header_buffer + offset, sizeof(header_buffer) - offset,
					   "Content-Length: %lu\r\n\r\n", (unsigned long)res->body_len);
		if (ret > 0)
			offset += (size_t)ret;

		send(client_socket, header_buffer, offset, 0);
		if (res->body && res->body_len > 0)
		{
			send(client_socket, res->body, res->body_len, 0);
		}
	}
}

void http_send_error(int client_socket, int status_code, const char *status_msg, const char *body)
{
	http_response_t res = {};
	res.status_code = status_code;
	res.status_message = status_msg;
	res.content_type = "text/plain";
	res.body = body;
	res.body_len = strlen(body);
	http_send_response(client_socket, &res);
}

static void serve_static_file(int client_socket, http_request_t *req)
{
	char target_file[PATH_BUFFER_SIZE];
	const char *relative_path = strcmp(req->path, "/") == 0 ? "index.html" : req->path + 1;

	snprintf(target_file, sizeof(target_file), "%s%s", STATIC_DIR, relative_path);

	struct stat st_file;
	if (stat(target_file, &st_file) == -1)
	{
		char temp_file[PATH_BUFFER_SIZE];
		snprintf(temp_file, sizeof(temp_file), "%s%s.html", STATIC_DIR, relative_path);

		if (stat(temp_file, &st_file) == 0)
		{
			snprintf(target_file, sizeof(target_file), "%s%s.html", STATIC_DIR, relative_path);
		}
	}

	if (stat(target_file, &st_file) == -1)
	{
		http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Página não encontrada.");
		return;
	}

	http_response_t res = {};
	res.status_code = HTTP_STATUS_OK;
	res.status_message = "OK";
	res.content_type = get_mime_type(target_file);
	res.file_path = target_file;
	http_send_response(client_socket, &res);
}

void http_handle_client(int client_socket)
{
	static bool dirs_checked = false;
	if (!dirs_checked)
	{
		ensure_static_directory();
		dirs_checked = true;
	}

	char buffer[RECV_BUFFER_SIZE] = {};
	size_t total_received = 0;
	char *body_separator = NULL;

	while (total_received < sizeof(buffer) - 1)
	{
		ssize_t bytes_received = recv(client_socket, buffer + total_received,
									  sizeof(buffer) - 1 - total_received, 0);

		if (bytes_received <= 0)
			break;

		total_received += (size_t)bytes_received;
		buffer[total_received] = '\0';

		body_separator = strstr(buffer, "\r\n\r\n");
		if (body_separator != NULL)
		{
			break;
		}
	}

	if (total_received <= 0 || body_separator == NULL)
	{
		return;
	}

	*body_separator = '\0';

	char *partial_body_start = body_separator + 4;
	size_t header_length = (size_t)(partial_body_start - buffer);

	http_request_t req = {};
	parse_http_request(buffer, &req);

	for (size_t i = 0; i < req.header_count; i++)
	{
		if (strcasecmp(req.headers[i].key, "Content-Length") == 0)
		{
			req.body_len = (size_t)strtoul(req.headers[i].value, NULL, 10);
			break;
		}
	}

	printf("[INFO] Method: %d | Path: %s | Content-Length: %zu\n", req.method, req.path, req.body_len);

	if (req.body_len > 0)
	{
		req.body = (char *)malloc(req.body_len + 1);
		if (!req.body)
		{
			printf("[ERRO] Falha de alocacao de memoria para o corpo (Out of Memory).\n");
			http_send_error(client_socket, 500, "Internal Server Error", "Memory full");
			return;
		}

		size_t bytes_already_read_for_body = total_received - header_length;
		if (bytes_already_read_for_body > 0)
		{
			memcpy(req.body, partial_body_start, bytes_already_read_for_body);
		}

		size_t current_body_bytes = bytes_already_read_for_body;
		while (current_body_bytes < req.body_len)
		{
			size_t bytes_to_read = req.body_len - current_body_bytes;

			ssize_t bytes = recv(client_socket,
								 req.body + current_body_bytes,
								 bytes_to_read, 0);

			if (bytes <= 0)
			{
				printf("[WARN] Cliente fechou a conexao TCP prematuramente durante o upload.\n");
				break;
			}

			current_body_bytes += (size_t)bytes;
		}

		req.body[current_body_bytes] = '\0';
	}

	if (req.path != NULL)
	{
		if (strstr(req.path, "..") != NULL)
		{
			http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso Negado.");
		}
		else if (!api_handle_request(client_socket, &req))
		{
			if (req.method == HTTP_GET)
			{
				serve_static_file(client_socket, &req);
			}
			else
			{
				http_send_error(client_socket, HTTP_STATUS_NOT_ALLOWED, "Method Not Allowed", "Método não suportado.");
			}
		}
	}

	if (req.body != NULL)
	{
		free(req.body);
	}
}
