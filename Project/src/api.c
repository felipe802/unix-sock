#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api.h"
#include "http.h"

constexpr char DATA_DIR[] = "./data/";
constexpr char API_DATA_PREFIX[] = "/api/data/";
constexpr char API_FILES_ROUTE[] = "/api/files";
constexpr char ROUTE_EDITOR[] = "/editor";
constexpr char ROUTE_EDITOR_HTML[] = "/editor.html";

constexpr char AUTH_CREDENTIALS[] = "Basic YWRtaW46YWRtaW4=";
constexpr char AUTH_REALM[] = "Area Restrita do Admin";

constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t RESP_BUFFER_SIZE = 8192;
constexpr size_t AUTH_HEADER_BUFFER_SIZE = 1024;

static void ensure_data_directory()
{
	struct stat st = {};
	if (stat(DATA_DIR, &st) == -1)
		mkdir(DATA_DIR, 0755);
}

static bool requires_auth(http_request_t *req)
{
	if (req->method == HTTP_DELETE)
		return true;

	if (strcmp(req->path, ROUTE_EDITOR) == 0 || strcmp(req->path, ROUTE_EDITOR_HTML) == 0)
	{
		return true;
	}
	return false;
}

static bool is_authenticated(http_request_t *req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcmp(req->headers[i].key, "Authorization") == 0)
		{
			if (strcmp(req->headers[i].value, AUTH_CREDENTIALS) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

static void api_handle_get(int client_socket, http_request_t *req)
{
	if (strcmp(req->path, API_FILES_ROUTE) == 0)
	{
		DIR *dir;
		struct dirent *ent;
		char json_list[RESP_BUFFER_SIZE] = "[";
		size_t offset = 1;

		if ((dir = opendir(DATA_DIR)) != nullptr)
		{
			while ((ent = readdir(dir)) != nullptr)
			{
				if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
					continue;

				char filepath[PATH_BUFFER_SIZE];
				snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

				struct stat st;
				if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode))
				{
					if (offset > 1)
					{
						offset += (size_t)snprintf(json_list + offset, sizeof(json_list) - offset, ",");
					}
					offset += (size_t)snprintf(json_list + offset, sizeof(json_list) - offset, "\"%s\"", ent->d_name);
				}
			}
			closedir(dir);
		}
		snprintf(json_list + offset, sizeof(json_list) - offset, "]");

		http_response_t res = {};
		res.status_code = HTTP_STATUS_OK;
		res.status_message = "OK";
		res.content_type = "application/json";
		res.body = json_list;
		res.body_len = strlen(json_list);
		http_send_response(client_socket, &res);
		return;
	}

	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		const char *filename = req->path + strlen(API_DATA_PREFIX);
		if (strchr(filename, '/') != nullptr)
		{
			http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Subdiretórios não permitidos.");
			return;
		}

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

		struct stat st;
		if (stat(filepath, &st) == -1)
		{
			http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Arquivo não encontrado.");
			return;
		}

		http_response_t res = {};
		res.status_code = HTTP_STATUS_OK;
		res.status_message = "OK";
		res.content_type = "text/plain";
		res.file_path = filepath;
		http_send_response(client_socket, &res);
		return;
	}

	http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota de API inexistente.");
}

static void api_handle_post(int client_socket, http_request_t *req)
{
	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		const char *filename = req->path + strlen(API_DATA_PREFIX);
		if (strchr(filename, '/') != nullptr)
			return;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

		FILE *f = fopen(filepath, "w");
		if (f)
		{
			if (req->body)
				fwrite(req->body, 1, strlen(req->body), f);
			fclose(f);
		}

		http_response_t res = {};
		res.status_code = HTTP_STATUS_OK;
		res.status_message = "OK";
		res.content_type = "text/plain";
		res.body = "Criado/Atualizado com sucesso (POST)!";
		res.body_len = strlen(res.body);
		http_send_response(client_socket, &res);
		return;
	}

	http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota POST desconhecida.");
}

static void api_handle_put(int client_socket, http_request_t *req)
{
	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		const char *filename = req->path + strlen(API_DATA_PREFIX);
		if (strchr(filename, '/') != nullptr)
			return;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

		FILE *f = fopen(filepath, "w");
		if (f)
		{
			if (req->body)
				fwrite(req->body, 1, strlen(req->body), f);
			fclose(f);
		}

		http_response_t res = {};
		res.status_code = HTTP_STATUS_OK;
		res.status_message = "OK";
		res.content_type = "text/plain";
		res.body = "Substituído com sucesso (PUT)!";
		res.body_len = strlen(res.body);
		http_send_response(client_socket, &res);
		return;
	}

	http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota PUT desconhecida.");
}

static void api_handle_patch(int client_socket, http_request_t *req)
{
	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		const char *filename = req->path + strlen(API_DATA_PREFIX);
		if (strchr(filename, '/') != nullptr)
			return;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

		FILE *f = fopen(filepath, "a");
		if (f)
		{
			if (req->body)
				fwrite(req->body, 1, strlen(req->body), f);
			fclose(f);
		}

		http_response_t res = {};
		res.status_code = HTTP_STATUS_OK;
		res.status_message = "OK";
		res.content_type = "text/plain";
		res.body = "Modificado parcialmente com sucesso (PATCH)!";
		res.body_len = strlen(res.body);
		http_send_response(client_socket, &res);
		return;
	}

	http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota PATCH desconhecida.");
}

static void api_handle_delete(int client_socket, http_request_t *req)
{
	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		const char *filename = req->path + strlen(API_DATA_PREFIX);
		if (strchr(filename, '/') != nullptr)
			return;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

		if (remove(filepath) == 0)
		{
			http_send_error(client_socket, HTTP_STATUS_OK, "OK", "Arquivo apagado com sucesso (DELETE).");
		}
		else
		{
			http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Erro ao apagar arquivo.");
		}
		return;
	}
	http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota DELETE desconhecida.");
}

bool api_handle_request(int client_socket, http_request_t *req)
{
	static bool dirs_checked = false;
	if (!dirs_checked)
	{
		ensure_data_directory();
		dirs_checked = true;
	}

	if (requires_auth(req) && !is_authenticated(req))
	{
		char header_buffer[AUTH_HEADER_BUFFER_SIZE];
		const char *body = "Não autorizado. Credenciais incorretas ou ausentes.";

		snprintf(header_buffer, sizeof(header_buffer),
		         "HTTP/1.1 401 Unauthorized\r\n"
		         "WWW-Authenticate: Basic realm=\"%s\"\r\n"
		         "Content-Length: %zu\r\n"
		         "Connection: close\r\n\r\n"
		         "%s",
		         AUTH_REALM, strlen(body), body);

		send(client_socket, header_buffer, strlen(header_buffer), 0);
		return true;
	}

	if (strncmp(req->path, "/api/", 5) != 0)
	{
		return false;
	}

	switch (req->method)
	{
	case HTTP_GET:
		api_handle_get(client_socket, req);
		break;
	case HTTP_POST:
		api_handle_post(client_socket, req);
		break;
	case HTTP_PUT:
		api_handle_put(client_socket, req);
		break;
	case HTTP_PATCH:
		api_handle_patch(client_socket, req);
		break;
	case HTTP_DELETE:
		api_handle_delete(client_socket, req);
		break;
	default:
		http_send_error(client_socket, HTTP_STATUS_NOT_ALLOWED, "Method Not Allowed", "Método não suportado na API.");
		break;
	}

	return true;
}
