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

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

struct http_context {
	char base_path[16];
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

/**
 * @brief Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path)
 *
 * @param dest
 * @param base_path
 * @param uri
 * @param destsize
 * @return const char*
 */
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

static esp_err_t http_404_error_handler(
	httpd_req_t *req, httpd_err_code_t err
) {
	char *buffer = NULL;
	read_file(&buffer, "/spiffs/404.html");
	if (!buffer) {
		httpd_resp_send_err(
			req, err, "<h1>404 NOT FOUND</h1>");
		return ESP_FAIL;
	}
	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, buffer);
	free(buffer);

	return ESP_FAIL;
}

/**
 * @brief processing http url query of config settings
 *
 * @param req [in] http request
 * @param has_query [out] has HTTP query
 * @return esp_err_t
 */
static esp_err_t process_settings_query(httpd_req_t *req, bool *has_query) {
	if (req == NULL || has_query == NULL) {
		ESP_LOGE(TAG, "process_settings_query: invalid param");
		return ESP_FAIL;
	}

	// Read URL query string length and allocate memory for
	// length + 1, extra byte for null termination.
	int length = httpd_req_get_url_query_len(req) + 1;
	if (length < 1) {
		// Query not found, return directly.
		return ESP_OK;
	}

	int ret = 0;
	char *buffer = malloc(length);
	if ((ret = httpd_req_get_url_query_str(req, buffer, length)) != 0) {
		if (ret != ESP_ERR_NOT_FOUND) {
			free(buffer);
			ESP_LOGE(TAG, "httpd_req_get_url_query_str: %d", ret);
			return ret;
		}
	}

	char param[128] = { 0 };
	static const char keys[][24] = {
		CONFIG_KEY_PWM_FAN_CHANNEL,
		CONFIG_KEY_PWM_FAN_FREQUENCY,
		CONFIG_KEY_PWM_FAN_GPIO,
		CONFIG_KEY_PWM_FAN_DUTY,
		CONFIG_KEY_PWM_MOS_CHANNEL,
		CONFIG_KEY_PWM_MOS_FREQUENCY,
		CONFIG_KEY_PWM_MOS_GPIO,
		CONFIG_KEY_PWM_MOS_DUTY,
		CONFIG_KEY_WIFI_SSID,
		CONFIG_KEY_WIFI_PASSWORD,
		CONFIG_KEY_WIFI_CHANNEL,
		CONFIG_KEY_DHCPS_IP,
		CONFIG_KEY_DHCPS_NETMASK,
		CONFIG_KEY_DHCPS_AS_ROUTER
	};
	static int keys_num = sizeof(keys) / (sizeof(char) * 24);
	for (int i = 0; i < keys_num; i++) {
		if (httpd_query_key_value(buffer, (const char*) &keys[i],
			param, sizeof(param)) == 0)
		{
			ESP_LOGI(TAG, "process_settings_query: "
				"query setting: %s=%s", keys[i], param);
			ret = global_controller_update_config(
				(const char*) &keys[i], param);
			if (ret != ESP_OK) {
				ESP_LOGE(TAG, "process_settings_query: "
					"failed to update setting %s: %d",
					keys[i], ret);
				free(buffer);
				return ret;
			}
			*has_query = true;
		}
	}

	free(buffer);
	return 0;
}

/**
 * @brief handler '/settings' http get request.
 * It will update the controller config by http get query,
 * and the response will be the updated config json data.
 *
 * @param req
 * @return esp_err_t
 */
static esp_err_t handle_http_settings_req(httpd_req_t *req)
{
	int ret = 0;
	bool has_query = false;
	ret = process_settings_query(req, &has_query);
	if (ret != ESP_OK) {
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: process_settings_query failed"
		);
	}

	char *data = malloc(sizeof(char) * 1024);
	if (data == NULL) {
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: malloc failed when marshal json"
		);
	}
	memset(data, 0, sizeof(char) * 1024);
	if ((ret = global_controller_config_marshal_json(data)) != ESP_OK) {
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: config_marshal_json failed"
		);
	}
	if ((ret = httpd_resp_set_type(req, "application/json")) != ESP_OK) {
		free(data);
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: httpd_resp_set_type failed"
		);
	}
	if ((ret = global_controller_apply_pwm_duty()) != ESP_OK) {
		free(data);
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: global_controller_apply_pwm_duty failed"
		);
	}
	if (has_query && (ret = global_controller_save_config()) != ESP_OK) {
		free(data);
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"500: global_controller_save_config failed"
		);
	}
	ret = httpd_resp_send(req, data, HTTPD_RESP_USE_STRLEN);
	free(data);
	data = NULL;
	ESP_LOGD(TAG, "handle_http_settings_req: response config json");

	return ret;
}

static esp_err_t handle_http_restart_req(httpd_req_t *req)
{
	int ret = 0;
	ret = httpd_resp_send(req, "SUCCEED", HTTPD_RESP_USE_STRLEN);
	global_controller_stop();

	return ret;
}

static esp_err_t handle_http_reset_settings_req(httpd_req_t *req)
{
	int ret = 0;
	ret = httpd_resp_send(req, "SUCCEED", HTTPD_RESP_USE_STRLEN);
	global_controller_reset_default();
	return ret;
}

/**
 * @brief default handler for handling all requests.
 * by default this handler will try to load the static html file.
 * If the URI is '/settings', the handler will update the controller config
 * by the settings http query and response the JSON settings data.
 *
 * @param req
 * @return esp_err_t
 */
static esp_err_t http_default_handler(httpd_req_t *req)
{
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
		return httpd_resp_send_err(
			req,
			HTTPD_414_URI_TOO_LONG,
			"URI TOO LONG"
		);
	}

	// Only GET method allowed for other URLs
	if (req->method != HTTP_GET) {
		return httpd_resp_send_err(
			req,
			HTTPD_405_METHOD_NOT_ALLOWED,
			"METHOD NOT ALLOWED"
		);
	}

	if (strcmp(filename, "/settings") == 0) {
		return handle_http_settings_req(req);
	}
	if (strcmp(filename, "/restart") == 0) {
		return handle_http_restart_req(req);
	}
	if (strcmp(filename, "/reset_settings") == 0) {
		return handle_http_reset_settings_req(req);
	}

	char *buffer = malloc(2048 * sizeof(char));
	if (buffer == NULL) {
		return httpd_resp_send_err(
			req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"http_default_handler: malloc failed"
		);
	}
	if (is_regular_file(filepath)) {
		// the file exists and is not a directory.
		sprintf(buffer, "%s", filepath);
	} else {
		// the filepath may not exists or maybe a directory.
		if (filepath[strlen(filepath)-1] == '/') {
			// remove the last '/' in filepath.
			filepath[strlen(filepath)-1] = '\0';
		}
		sprintf(buffer, "%s/index.html", filepath);
	}

	char *content = NULL;
	int size = read_file(&content, buffer);
	if (!content) {
		free(buffer);
		return http_404_error_handler(req, HTTPD_404_NOT_FOUND);
	}
	ret = set_content_type_from_file(req, buffer);
	if (ret != ESP_OK) {
		free(buffer);
		free(content);
		content = NULL;
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
		return httpd_resp_send_err(req,
			HTTPD_500_INTERNAL_SERVER_ERROR,
			"set_content_type_from_file failed");
	}
	ret = httpd_resp_send(req, content, size);
	ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
	free(buffer);
	free(content);
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
