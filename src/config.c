#include <string.h>
#include <esp_log.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_netif.h>

#include "config.h"
#include "storage.h"

#define TAG "CONFIG"
#define CONFIG_FILE "/spiffs/config/config.cfg"

struct pwm_config {
	uint8_t channel;
	uint32_t frequency;
	uint8_t gpio;
	uint8_t duty;
};

struct wifi_config {
	char* ssid;
	char* password;
	uint8_t channel;
};

struct dhcps_config {
	esp_ip4_addr_t ip;      /**< Interface IPV4 address */
	esp_ip4_addr_t netmask; /**< Interface IPV4 netmask */
	esp_ip4_addr_t gw;      /**< Interface IPV4 gateway address */
	uint8_t as_router;
};

struct config {
	struct pwm_config *pwm;
	struct wifi_config *wifi;
	struct dhcp_config *dhcp;
};

const struct config* process_config_content(const char *content, int length);
bool is_valid_config_value(char c);

esp_err_t get_controller_config()
{
	char *content = NULL;
	int ret = read_file(&content, CONFIG_FILE);
	if (content == NULL || ret <= 0) {
		ESP_LOGE(TAG, "failed to open config %s: %d", CONFIG_FILE, ret);
		return ESP_FAIL;
	}
	process_config_content(content, ret);
	return 0;
}

const struct config* process_config_content(const char *content, int length)
{
	if (content == NULL) {
		ESP_LOGE(TAG, "failed to validate content: content is NULL");
		return false;
	}

	char *key = malloc(128 * sizeof(char));
	char *value = malloc(128 * sizeof(char));
	memset(key, 0, 128 * sizeof(char));
	memset(value, 0, 128 * sizeof(char));

	int key_pos = 0;
	int value_pos = 0;
	bool is_value = false;

	for (int i = 0; i < length; ++i) {
		if (content[i] == '\0') {
			break;
		}
		if (content[i] == '=') {
			if (is_value) {
				return false;
			}
			is_value = true;
			value_pos = i+1;
			if (value_pos >= length) {
				break;
			}
			memcpy(key, content + key_pos, i-key_pos);
			key[i-key_pos] = '\0';
			continue;
		}
		if (content[i] == '\n' || content[i] == '\0' || i >= length) {
			is_value = false;
			key_pos = i+1;
			if (value_pos >= length) {
				break;
			}
			memcpy(value, content + value_pos, i - value_pos);
			value[i-value_pos] = '\0';
			ESP_LOGI(TAG, "read key [%s] value [%s]", key, value);
			key[0] = '\0';
			value[0] = '\0';
			value_pos = key_pos;
			continue;
		}
		if (is_value && !is_valid_config_value(content[i])) {
			ESP_LOGE(TAG, "invalid value char [%c] on key [%s]",
				content[i], key);
		}
	}
	return true;
}

int config_set_key_value(
	struct config *config, const char *key, const char *value
){
	if (config == NULL || key == NULL || value == NULL) {
		return ESP_FAIL;
	}
	if (config->dhcp == NULL || config->pwm == NULL
		|| config->wifi == NULL) {
		return ESP_FAIL;
	}
	if (strcmp(key, "pwm_channel") == 0) {
		uint8_t v = value[0] - '0';
		if (v > 5) {
			ESP_LOGE(TAG, "invalid PWM channel [%d], "
				"set to default 0", v);
			v = 0;
		}
		ESP_LOGI(TAG, "get pwm_channel %d", v);
		config->pwm->channel = v;
		return 0;
	}
	if (strcmp(key, "pwm_frequency") == 0) {
		int length = strlen(value);
		int v = 0;
		for (int i = 0; i < length; i++) {
			if (value[i] > '9' || value[i] < '0') {
				continue;
			}
			v = v * 10 + value[i] - '0';
		}
		if (v > 10000 || v < 1000) {
			ESP_LOGE(TAG, "invalid PWM frequency [%d], "
				"set to default 2500", v);
			v = 2500;
		}
		ESP_LOGI(TAG, "get pwm_frequency %d", v);
		config->pwm->frequency = v;
		return 0;
	}
	if (strcmp(key, "pwm_gpio") == 0) {
		int length = strlen(value);
		int v = 0;
		for (int i = 0; i < length; i++) {
			if (value[i] > '9' || value[i] < '0') {
				continue;
			}
			v = v * 10 + value[i] - '0';
		}
		if (v > 30 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_gpio [%d], "
				"set to default 4", v);
			v = 4;
		}
		ESP_LOGI(TAG, "get pwm_gpio %d", v);
		config->pwm->gpio = v;
		return 0;
	}
	return 0;
}

bool is_valid_config_value(char c) {
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9')) {
		return true;
	}

	static const char *const valid = "!@#$%%^&*()-_+.";
	int l = strlen(valid);
	for (int i = 0; i < l; ++i) {
		if (c == valid[i]) {
			return true;
		}
	}
	return false;
}
