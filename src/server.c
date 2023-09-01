#include <esp_log.h>

#include "server.h"

#define TAG "SERVER"

static esp_err_t hello_get_handler(httpd_req_t *req)
{
	char*  buffer = NULL;
	size_t length = 0;
	esp_err_t ret = ESP_OK;

	// Host
	length = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (length > 1) {
		buffer = malloc(length);
		/* Copy null terminated value string into buffer */
		ret = httpd_req_get_hdr_value_str(req, "Host", buffer, length);
		if (ret == ESP_OK) {
			ESP_LOGI(TAG, "Hello from host: %s", buffer);
		}
		free(buffer);
		buffer = NULL;
	}

	// Query
	// length = httpd_req_get_url_query_len(req) + 1;
	// if (length > 1) {
	// 	buffer = malloc(length);
	// 	if (httpd_req_get_url_query_str(req, buffer, length) == ESP_OK) {
	// 		ESP_LOGI(TAG, "Found URL query:\n%s", buffer);
	// 		char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
	// 		/* Get value of expected key from query string */
	// 		if (httpd_query_key_value(buffer, "query1", param, sizeof(param)) == ESP_OK) {
	// 			ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
	// 			example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
	// 			ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
	// 		}
	// 		if (httpd_query_key_value(buffer, "query3", param, sizeof(param)) == ESP_OK) {
	// 			ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
	// 			example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
	// 			ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
	// 		}
	// 		if (httpd_query_key_value(buffer, "query2", param, sizeof(param)) == ESP_OK) {
	// 			ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
	// 			example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
	// 			ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
	// 		}
	// 	}
	// 	free(buffer);
	// }

	/* Set some custom headers */
	// httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	// httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	/* Send response with custom headers and body set as the
	 * string passed in user context*/
	const char* resp_str = (const char*) req->user_ctx;
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

	/* After sending the HTTP response the old HTTP request
	 * headers are lost. Check if HTTP request headers can be read now. */
	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
		ESP_LOGI(TAG, "Request headers lost");
	}

	return ESP_OK;
}