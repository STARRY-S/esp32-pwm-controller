#include <esp_log.h>
#include <esp_vfs.h>

#include "server.h"
#include "storage.h"
#include "controller.h"

#define TAG "SERVER"
#define HTTP_SERVER_PORT 80
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define SCRATCH_BUFSIZE  8192

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

struct http_context {
	char base_path[16];
	char scratch[SCRATCH_BUFSIZE];
	struct config *config;
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
	} else if (IS_FILE_EXT(filename, ".css")) {
		return httpd_resp_set_type(req, "text/css");
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
			HTTPD_404_NOT_FOUND,
			"<h1>404 NOT FOUND</h1>"
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
		if (httpd_req_get_url_query_str(req, buffer, length) != 0) {
			free(buffer);
			break;
		}
		// ESP_LOGI(TAG, "Found URL query => %s", buffer);
		remove_query = true;
		char param[128] = { 0 };
		if (httpd_query_key_value(
			buffer, "fan-enable", param, sizeof(param)) == 0) {
			ESP_LOGI(TAG,
				"Found URL query parameter => fan-enable=%s",
				param);
                        ESP_LOGI(TAG, "fan-enable: %s", buffer);
			if (strcmp(param, "0") == 0) {
				// int ret = controller_set_pwm_fan_duty(0);
				// if (ret != ESP_OK) {
				// 	ESP_LOGE(TAG,
				// 		"failed to set duty: %d", ret);
				// }
				free(buffer);
				buffer = NULL;
				break;
			}
		}
		if (httpd_query_key_value(
			buffer, "fan-speed", param, sizeof(param)) == 0) {
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
				duty = 255;
			}
			// int ret = controller_set_pwm_fan_duty(duty);
			// if (ret != ESP_OK) {
			// 	free(buffer);
			// 	buffer = NULL;
			// 	ESP_LOGE(TAG,
			// 		"failed to set duty: %d", ret);
			// 	break;
			// }
                        ESP_LOGI(TAG, "fan-speed: %d", duty);
		}
		if (httpd_query_key_value(
			buffer, "getconfig", param, sizeof(param)) == 0) {
			free(buffer);
			buffer = NULL;

			uint8_t duty = 0;
			int ret = 0;
			// int ret = controller_get_pwm_fan_duty(&duty);
			// if (ret != ESP_OK) {
			// 	ESP_LOGE(TAG,
			// 		"failed to get duty: %d", ret);
			// 	break;
			// }
			buffer = malloc(sizeof(char) * 128);
			sprintf(buffer, "{\"duty\":%u}", duty);
			ret = httpd_resp_set_type(req, "application/json");
			if (ret != ESP_OK) {
				return ret;
			}
			ret = httpd_resp_send(req, buffer, -1);
			ESP_LOGI(TAG, "getconfig: %s", buffer);
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
"<link rel='stylesheet' href='/css/styles.css'>\n"
"<meta http-equiv='Refresh' content=\"0; url='/controller'\" />\n"
"</head>\n"
"<body></body>\n",
			HTTPD_RESP_USE_STRLEN);
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
	static char filepath[CONFIG_HTTPD_MAX_URI_LEN] = { 0 };
	memset(filepath, 0, CONFIG_HTTPD_MAX_URI_LEN * sizeof(char));
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
			HTTPD_414_URI_TOO_LONG,
			"URI TOO LONG"
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

httpd_uri_t* default_get_handler(struct config *config)
{
	static struct http_context *http_context = NULL;
	static httpd_uri_t *http_get_handler = NULL;
	if (http_context && http_get_handler) {
		return http_get_handler;
	}

	http_context = malloc(sizeof(struct http_context));
	http_get_handler = malloc(sizeof(httpd_uri_t));
	memset(http_context, 0, sizeof(struct http_context));
	memset(http_get_handler, 0, sizeof(httpd_uri_t));
	strcpy(http_context->base_path, "/spiffs");

	http_context->config = config;
	http_get_handler->uri = "/*";
	http_get_handler->method = HTTP_GET;
	http_get_handler->handler = http_default_handler;
	http_get_handler->user_ctx = http_context;
	return http_get_handler;
}

esp_err_t start_default_http_server(
	httpd_handle_t *handle,
	struct config *config
) {
	if (!handle || !config) {
		ESP_LOGE(TAG, "start_default_http_server: param is NULL");
		return ESP_FAIL;
	}
	httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
	http_config.lru_purge_enable = true;
	http_config.server_port = HTTP_SERVER_PORT;

	/*
	 * Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme
	 */
	http_config.uri_match_fn = httpd_uri_match_wildcard;
	int ret = 0;
	httpd_handle_t server = NULL;
	ret = httpd_start(&server, &http_config);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_start_server: httpd_start failed: "
			"[%d]", ret);
		return ret;
	}

	httpd_uri_t *http_get_handler = default_get_handler(config);
	ESP_LOGD(TAG, "register default handler for server");
	ret = httpd_register_uri_handler(server, http_get_handler);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_start_server: "
			"httpd_register_uri_handler failed: [%d]", ret);
		httpd_stop(server);
		return ret;
	}
	ESP_LOGD(TAG, "register error handler for server");
	ret = httpd_register_err_handler(
		server, HTTPD_404_NOT_FOUND, http_404_error_handler
	);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_start_server: "
			"httpd_register_err_handler failed: [%d]", ret);
		httpd_stop(server);
		return ret;
	}
	ESP_LOGI(TAG, "started controller server on port [%u]",
		(unsigned int) http_config.server_port);

	*handle = server;
 	return ESP_OK;
}

esp_err_t stop_default_http_server(httpd_handle_t h)
{
	return httpd_stop(h);
}
