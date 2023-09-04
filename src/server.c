#include <esp_log.h>
#include <esp_vfs.h>

#include "server.h"
#include "storage.h"
#include "controller.h"

#define TAG "SERVER"
#define MIN( a , b ) ( a < b ) ? ( a ) : ( b )

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN 64
#define SCRATCH_BUFSIZE  8192

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

struct http_context {
	char base_path[16];
	char scratch[SCRATCH_BUFSIZE];
};

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(
	httpd_req_t *req, const char *filename
) {
	if (IS_FILE_EXT(filename, ".pdf")) {
		return httpd_resp_set_type(req, "application/pdf");
	} else if (IS_FILE_EXT(filename, ".html")) {
		return httpd_resp_set_type(req, "text/html");
	} else if (IS_FILE_EXT(filename, ".jpeg")) {
		return httpd_resp_set_type(req, "image/jpeg");
	} else if (IS_FILE_EXT(filename, ".jpg")) {
		return httpd_resp_set_type(req, "image/jpeg");
	} else if (IS_FILE_EXT(filename, ".png")) {
		return httpd_resp_set_type(req, "image/png");
	} else if (IS_FILE_EXT(filename, ".ico")) {
		return httpd_resp_set_type(req, "image/x-icon");
	}
	return httpd_resp_set_type(req, "text/plain");
}


/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* get_path_from_uri(
	char *dest, const char *base_path, const char *uri, size_t destsize
) {
	const size_t base_pathlen = strlen(base_path);
	size_t pathlen = strlen(uri);

	const char *quest = strchr(uri, '?');
	if (quest) {
		pathlen = MIN(pathlen, quest - uri);
	}
	const char *hash = strchr(uri, '#');
	if (hash) {
		pathlen = MIN(pathlen, hash - uri);
	}

	if (base_pathlen + pathlen + 1 > destsize) {
		/* Full path string won't fit into destination buffer */
		return NULL;
	}

	/* Construct full path (base + path) */
	strcpy(dest, base_path);
	strlcpy(dest + base_pathlen, uri, pathlen + 1);

	/* Return pointer to path, skipping the base */
	return dest + base_pathlen;
}

static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
	char *buffer = NULL;
	read_file(&buffer, "/spiffs/404.html");
	if (!buffer) {
		httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"failed to open '404.html'"
		);
		return ESP_FAIL;
	}
	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, buffer);
	free(buffer);

	return ESP_FAIL;
}

static esp_err_t http_controller_handler(httpd_req_t *req)
{
	bool remove_query = false;
	while (1) {
		/* Read URL query string length and allocate memory for
		* length + 1, extra byte for null termination.
		*/
		int length = httpd_req_get_url_query_len(req) + 1;
		if (length < 1) {
			break;
		}

		char *buffer = malloc(length);
		if (httpd_req_get_url_query_str(req, buffer, length) != ESP_OK)
		{
			free(buffer);
			break;
		}
		ESP_LOGI(TAG, "Found URL query => %s", buffer);
		remove_query = true;
		char param[128] = { 0 };
		if (httpd_query_key_value(
			buffer, "fan-enable", param, sizeof(param)) == ESP_OK) {
			ESP_LOGI(TAG,
				"Found URL query parameter => fan-enable=%s",
				param);
			if (strcmp(param, "0") == 0) {
				int ret = controller_set_pwm_duty(0);
				if (ret != ESP_OK) {
					ESP_LOGE(TAG,
						"failed to set duty: %d", ret);
				}
				free(buffer);
				buffer = NULL;
				break;
			}
		}
		if (httpd_query_key_value(
			buffer, "fan-speed", param, sizeof(param)) == ESP_OK) {
			ESP_LOGI(TAG,
				"Found URL query parameter => fan-speed=%s",
				param);
			int duty = 0;
			int param_length = strlen(param);
			for (int i = 0; i < param_length; i++) {
				if (param[i] > '9' || param[i] < '0') {
					continue;
				}
				duty = duty * 10 + param[i] - '0';
			}
			if (duty > 255) {
				ESP_LOGE(TAG, "duty outrange: %d", duty);
			} else {
				int ret = controller_set_pwm_duty(duty);
				if (ret != ESP_OK) {
					free(buffer);
					buffer = NULL;
					ESP_LOGE(TAG,
						"failed to set duty: %d", ret);
					break;
				}
			}
		}
		if (httpd_query_key_value(
			buffer, "getconfig", param, sizeof(param)) == ESP_OK) {
			free(buffer);
			buffer = NULL;

			uint8_t duty = 0;
			int ret = controller_get_pwm_duty(&duty);
			if (ret != ESP_OK) {
				ESP_LOGE(TAG,
					"failed to get duty: %d", ret);
				break;
			}
			buffer = malloc(sizeof(char) * 128);
			sprintf(buffer, "{\"duty\":%u}", duty);
			ret = httpd_resp_set_type(req, "application/json");
			if (ret != ESP_OK) {
				return ret;
			}
			ret = httpd_resp_send(req, buffer, -1);
			free(buffer);
			return ret;
		}

		free(buffer);
		break;
	}

	if (remove_query) {
		return httpd_resp_send(req,
"<!DOCTYPE html>\n"
"<head>\n"
"<meta http-equiv=\"Refresh\" content=\"0; url='/controller'\" />\n"
"</head>\n"
"<body></body>\n",
			-1);
	}

	char *content = NULL;
	int size = read_file(&content, "/spiffs/controller/index.html");
	if (!content) {
		return http_404_error_handler(req, HTTPD_404_NOT_FOUND);
	}
	esp_err_t ret = httpd_resp_set_type(req, "text/html");
	if (ret != ESP_OK) {
		free(content);
		content = NULL;
		return ret;
	}
	ret = httpd_resp_send(req, content, size);
	free(content);
	content = NULL;

	return ESP_OK;
}

static esp_err_t http_default_handler(httpd_req_t *req)
{
	static char buffer[1024];
	esp_err_t ret = ESP_OK;
	char filepath[128] = "";
	struct http_context *context = req->user_ctx;
	const char *filename = get_path_from_uri(
		filepath,
		context->base_path,
		req->uri,
		sizeof(filepath)
	);
	if (!filename) {
		httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"FILENAME TOO LONG"
		);
		return ESP_FAIL;
	}

	// Only GET method allowed for other URLs
	if (req->method != HTTP_GET) {
		return httpd_resp_send_err(
			req,
			HTTPD_405_METHOD_NOT_ALLOWED,
			"METHOD NOT ALLOWED"
		);
	}

	// Register handler for controller
	if (strcmp(filename, "/controller") == 0) {
		return http_controller_handler(req);
	}

	size_t length = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (length > 1) {
		ret = httpd_req_get_hdr_value_str(req, "Host", buffer, length);
		if (ret == ESP_OK) {
			ESP_LOGI(TAG, "HTTP get from host: %s", buffer);
		}
	}
	if (!is_regular_file(filepath)) {
		if (filepath[strlen(filepath)-1] == '/') {
			filepath[strlen(filepath)-1] = '\0';
		}
		sprintf(buffer, "%s/index.html", filepath);
	} else {
		sprintf(buffer, "%s", filepath);
	}
	char *content = NULL;
	int size = read_file(&content, buffer);
	if (!content) {
		return http_404_error_handler(req, HTTPD_404_NOT_FOUND);
	}
	ret = set_content_type_from_file(req, buffer);
	if (ret != ESP_OK) {
		free(content);
		content = NULL;
		return ret;
	}
	ret = httpd_resp_send(req, content, size);
	free(content);
	content = NULL;
	return ret;
}

httpd_handle_t start_webserver(uint16_t port)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.lru_purge_enable = true;
	config.server_port = port;

	/*
	 * Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme
	 */
	config.uri_match_fn = httpd_uri_match_wildcard;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(TAG, "Error starting server!");
		return NULL;
	}

	// Set URI handlers
	ESP_LOGI(TAG, "Registering URI handlers");
	static struct http_context context = {
		.base_path = "/spiffs"
	};
	static httpd_uri_t get_handler = {
		.uri = "/*",
		.method = HTTP_GET,
		.handler = http_default_handler,
		.user_ctx = &context
	};
	ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_handler));
	ESP_ERROR_CHECK(
		httpd_register_err_handler(
			server, HTTPD_404_NOT_FOUND, http_404_error_handler
		)
	);
	ESP_LOGI(TAG, "Web server started");

	return server;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
	return httpd_stop(server);
}
