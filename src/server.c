#include "server.h"
#include <esp_log.h>
#include <esp_vfs.h>

#define TAG "SERVER"
#define MIN( a , b ) ( a < b ) ? ( a ) : ( b )

#define SCRATCH_BUFSIZE  8192

struct http_context {
	char base_path[16];
	char scratch[SCRATCH_BUFSIZE];
};

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

static esp_err_t http_get_handler(httpd_req_t *req)
{
	static char buffer[256];
	size_t length = 0;
	esp_err_t ret = ESP_OK;

	char filepath[256] = "";
	struct http_context *context = req->user_ctx;

	if (strcmp(req->uri, "/ping") == 0) {
		httpd_resp_send(req, "PONG", HTTPD_RESP_USE_STRLEN);
		return ESP_OK;
	}

	const char *filename = get_path_from_uri(
		filepath,
		context->base_path,
		req->uri,
		sizeof(filepath)
	);

	if (!filename) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "FILENAME TOO LONG");
		return ESP_FAIL;
	}

	// Host
	length = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (length > 1) {
		/* Copy null terminated value string into buffer */
		ret = httpd_req_get_hdr_value_str(req, "Host", buffer, length);
		if (ret == ESP_OK) {
			ESP_LOGI(TAG, "HTTP get from host: %s", buffer);
		}
		memset(buffer, 0, sizeof(buffer));
	}


	sprintf(buffer, "<h1>filename: %s</h1>", filename);
	httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
	ESP_LOGI(TAG, "filepath: %s", filepath);

	/* After sending the HTTP response the old HTTP request
	 * headers are lost. Check if HTTP request headers can be read now. */
	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
		ESP_LOGD(TAG, "Connection lost");
	}

	return ESP_OK;
}

static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
	/* For any other URI send 404 and close socket */
	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 NOT FOUND");
	return ESP_FAIL;
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
		.handler = http_get_handler,
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
	// Stop the httpd server
	return httpd_stop(server);
}
