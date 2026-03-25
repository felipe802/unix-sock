#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "api.h"
#include "http.h"

constexpr char STATIC_DIR[] = "./web/";
constexpr size_t RECV_BUFFER_SIZE = 8192;
constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t HEADER_BUFFER_SIZE = 1024;
constexpr size_t FILE_CHUNK_SIZE = 8192;

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
	    {".jpeg", "image/jpeg"},
	    {".ico", "image/x-icon"}
	};

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
	    {"DELETE", HTTP_DELETE}
	};

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
	req->body = nullptr;
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

	while ((line = strtok_r(nullptr, "\r\n", &saveptr)) && req->header_count < MAX_HTTP_HEADERS)
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

	ret = snprintf(
	    header_buffer + offset,
	    sizeof(header_buffer) - offset,
	    "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nConnection: keep-alive\r\n",
	    res->status_code,
	    res->status_message,
	    res->content_type
	);
	if (ret > 0)
		offset += (size_t)ret;

	for (size_t i = 0; i < res->header_count && i < MAX_HTTP_HEADERS; i++)
	{
		ret = snprintf(
		    header_buffer + offset,
		    sizeof(header_buffer) - offset,
		    "%s: %s\r\n",
		    res->headers[i].key,
		    res->headers[i].value
		);
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

		ret = snprintf(
		    header_buffer + offset,
		    sizeof(header_buffer) - offset,
		    "Content-Length: %ld\r\n\r\n",
		    file_size
		);
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
		ret = snprintf(
		    header_buffer + offset,
		    sizeof(header_buffer) - offset,
		    "Content-Length: %lu\r\n\r\n",
		    (unsigned long)res->body_len
		);
		if (ret > 0)
			offset += (size_t)ret;

		send(client_socket, header_buffer, offset, 0);
		if (res->body && res->body_len > 0)
		{
			send(client_socket, res->body, res->body_len, 0);
		}
	}
}

void http_send_error(
    int client_socket, int status_code, const char *status_msg, const char *body
)
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
		http_send_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Página não encontrada."
		);
		return;
	}

	http_response_t res = {};
	res.status_code = HTTP_STATUS_OK;
	res.status_message = "OK";
	res.content_type = get_mime_type(target_file);
	res.file_path = target_file;
	http_send_response(client_socket, &res);
}

static void configure_socket_timeout(int client_socket, time_t seconds)
{
	struct timeval tv = {};
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static bool read_http_headers(
    int client_socket, char *buffer, size_t buffer_size, size_t *out_total_received,
    char **out_separator
)
{
	size_t total_received = 0;
	while (total_received < buffer_size - 1)
	{
		ssize_t bytes = recv(
		    client_socket, buffer + total_received, buffer_size - 1 - total_received, 0
		);
		if (bytes <= 0)
		{
			return false;
		}

		total_received += (size_t)bytes;
		buffer[total_received] = '\0';

		*out_separator = strstr(buffer, "\r\n\r\n");
		if (*out_separator != nullptr)
		{
			*out_total_received = total_received;
			return true;
		}
	}
	return false;
}

static size_t get_content_length(http_request_t *req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].key, "Content-Length") == 0)
		{
			return (size_t)strtoul(req->headers[i].value, nullptr, 10);
		}
	}
	return 0;
}

static bool read_http_body(
    int client_socket, http_request_t *req, const char *partial_body_start, size_t partial_len
)
{
	if (req->body_len == 0)
	{
		return true;
	}

	req->body = (char *)malloc(req->body_len + 1);
	if (!req->body)
	{
		return false;
	}

	if (partial_len > 0)
	{
		memcpy(req->body, partial_body_start, partial_len);
	}

	size_t current_len = partial_len;
	while (current_len < req->body_len)
	{
		ssize_t bytes = recv(
		    client_socket, req->body + current_len, req->body_len - current_len, 0
		);
		if (bytes <= 0)
		{
			return false;
		}
		current_len += (size_t)bytes;
	}

	req->body[current_len] = '\0';
	return true;
}

static void route_request(int client_socket, http_request_t *req)
{
	if (req->path == nullptr)
	{
		return;
	}

	if (strstr(req->path, "..") != nullptr)
	{
		http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso Negado.");
		return;
	}

	if (!api_handle_request(client_socket, req))
	{
		if (req->method == HTTP_GET)
		{
			serve_static_file(client_socket, req);
		}
		else
		{
			http_send_error(
			    client_socket,
			    HTTP_STATUS_NOT_ALLOWED,
			    "Method Not Allowed",
			    "Método não suportado."
			);
		}
	}
}

static bool should_keep_alive(http_request_t *req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].key, "Connection") == 0)
		{
			if (strcasecmp(req->headers[i].value, "close") == 0)
			{
				return false;
			}
		}
	}
	return true;
}

void http_handle_client(int client_socket)
{
	static bool dirs_checked = false;
	if (!dirs_checked)
	{
		ensure_static_directory();
		dirs_checked = true;
	}

	configure_socket_timeout(client_socket, 4);

	bool keep_alive = true;

	while (keep_alive)
	{
		char buffer[RECV_BUFFER_SIZE] = {};
		size_t total_received = 0;
		char *body_separator = nullptr;

		if (!read_http_headers(
		        client_socket, buffer, sizeof(buffer), &total_received, &body_separator
		    ))
		{
			break;
		}

		*body_separator = '\0';
		char *partial_body_start = body_separator + 4;
		size_t header_length = (size_t)(partial_body_start - buffer);

		http_request_t req = {};
		parse_http_request(buffer, &req);

		req.body_len = get_content_length(&req);

		printf(
		    "[INFO] Method: %d | Path: %s | Content-Length: %zu\n",
		    req.method,
		    req.path,
		    req.body_len
		);

		size_t partial_len = total_received - header_length;
		if (!read_http_body(client_socket, &req, partial_body_start, partial_len))
		{
			if (req.body_len > 0 && req.body == nullptr)
			{
				printf("[ERRO] Falha de alocacao de memoria para o corpo (Out of Memory).\n");
				http_send_error(client_socket, 500, "Internal Server Error", "Memory full");
			}
			else
			{
				printf(
				    "[WARN] Cliente fechou a conexao TCP prematuramente durante o upload.\n"
				);
			}

			if (req.body)
				free(req.body);
			break;
		}

		route_request(client_socket, &req);

		keep_alive = should_keep_alive(&req);

		if (req.body != nullptr)
		{
			free(req.body);
		}
	}
}
