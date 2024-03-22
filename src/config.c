#include <string.h>
#include <esp_log.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_netif.h>

#include "config.h"
#include "storage.h"
#include "utils.h"

#define TAG "CONFIG"
#define CONFIG_FILE "/spiffs/config/config.cfg"
#define BUFF_SIZE (128 * sizeof(char))

struct pwm_config {
	uint8_t channel;    /* PWM Channel */
	uint32_t frequency; /* PWM Frequency */
	uint8_t gpio;       /* GPIO PIN (fan pwm pin) */
	uint8_t duty;       /* PWM duty (fan speed) */
};

struct wifi_config {
	char* ssid;      /* Wifi SSID */
	char* password;  /* Wifi Password */
	uint8_t channel; /* Wifi Channel */
};

struct dhcps_config {
	esp_ip4_addr_t ip;      /* Interface IPV4 address */
	esp_ip4_addr_t netmask; /* Interface IPV4 netmask */
	uint8_t as_router;      /* Set default route for connected devices */
};

struct config {
	struct pwm_config *pwm;
	struct wifi_config *wifi;
	struct dhcps_config *dhcps;
};

const struct config* process_config_content(const char *content, int length);
bool is_valid_config_value(char c);
int config_set_key_value(
	struct config *config, const char *key, const char *value);

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

struct config* new_config_default_value()
{
	struct config *config = malloc(sizeof(struct config));
	if (config == NULL) {
		ESP_LOGE(TAG, "new_config_default_value failed: malloc fail");
		return NULL;
	}
	config->pwm = malloc(sizeof(struct pwm_config));
	config->wifi = malloc(sizeof(struct wifi_config));
	config->dhcps = malloc(sizeof(struct dhcps_config));
	if (!config->pwm || !config->wifi || !config->dhcps) {
		free(config->pwm);
		free(config->wifi);
		free(config->dhcps);
		free(config);
		ESP_LOGE(TAG, "new_config_default_value failed: malloc fail");
		return NULL;
	}

	// Init config with default values.
	config->pwm->channel = 0;
	config->pwm->frequency = 25000;
	config->pwm->gpio = 4;
	config->pwm->duty = 100;
	config->wifi->ssid = str_clone("PWM_FAN_CONTROLLER");
	config->wifi->password = str_clone("testpassword123");
	config->wifi->channel = 1;
	config->dhcps->ip.addr = 0x010A0A0A; // 10.10.10.1
	config->dhcps->netmask.addr = 0x00ffffff; // 255.255.255.0
	config->dhcps->as_router = 0;
	return config;
}

const struct config* process_config_content(const char *content, int length)
{
	if (content == NULL) {
		ESP_LOGE(TAG, "process_config_content failed: content NULL");
		return NULL;
	}

	struct config *config = new_config_default_value();
	if (config == NULL) {
		return NULL;
	}

	char *key = malloc(BUFF_SIZE);
	char *value = malloc(BUFF_SIZE);
	if (key == NULL || value == NULL) {
		free(key);
		free(value);
		ESP_LOGE(TAG, "process_config_content failed: malloc failed");
		return NULL;
	}
	memset(key, 0, BUFF_SIZE);
	memset(value, 0, BUFF_SIZE);

	int key_pos = 0;
	int value_pos = 0;
	bool is_value = false;
	for (int i = 0; i < length; ++i) {
		if (content[i] == '\0') {
			break;
		}
		if (content[i] == '=') {
			if (is_value) {
				continue;
			}
			is_value = true;
			value_pos = i+1;
			if (value_pos >= length) {
				break;
			}
			if (i - key_pos > BUFF_SIZE-1) {
				free(key);
				free(value);
				ESP_LOGE(TAG, "process_config_content failed: "
					"key length out of range");
				return config;
			}
			memcpy(key, content + key_pos,
				(i-key_pos) * sizeof(char));
			key[i-key_pos] = '\0';
			continue;
		}
		if (content[i] == '\n' || content[i] == '\0' || i >= length) {
			is_value = false;
			key_pos = i+1;
			if (value_pos >= length) {
				break;
			}
			if (i - value_pos > BUFF_SIZE-1) {
				free(key);
				free(value);
				ESP_LOGE(TAG, "process_config_content failed: "
					"value length out of range");
				return config;
			}
			memcpy(value, content + value_pos, i - value_pos);
			value[i-value_pos] = '\0';
			ESP_LOGI(TAG, "read key [%s] value [%s]", key, value);
			int ret = config_set_key_value(config, key, value);\
			if (ret != ESP_OK) {
				ESP_LOGE(TAG, "config_set_key_value failed: "
					"key %s, value %s", key, value);
			}

			key[0] = '\0';
			value[0] = '\0';
			value_pos = key_pos;
			continue;
		}
	}
	return config;
}

int config_set_key_value(
	struct config *config, const char *key, const char *value
){
	if (config == NULL || key == NULL || value == NULL) {
		ESP_LOGE(TAG, "config_set_key_value failed: config NULL ptr");
		return ESP_FAIL;
	}
	if (!config->dhcps || !config->pwm || !config->wifi) {
		ESP_LOGE(TAG, "config_set_key_value failed: config NULL ptr");
		return ESP_FAIL;
	}
	if (strcmp(key, "pwm_channel") == 0) {
		int v = str2int(value);
		if (v > 5) {
			ESP_LOGE(TAG, "invalid PWM channel [%d], "
				"set to default 0", v);
			v = 0;
		}
		ESP_LOGI(TAG, "set config pwm_channel %d", v);
		config->pwm->channel = v;
		return 0;
	}
	if (strcmp(key, "pwm_frequency") == 0) {
		int v = str2int(value);
		if (v > 100000 || v < 1000) {
			ESP_LOGE(TAG, "invalid PWM frequency [%d], "
				"set to default 25000", v);
			v = 25000;
		}
		ESP_LOGI(TAG, "set config pwm_frequency %d", v);
		config->pwm->frequency = v;
		return 0;
	}
	if (strcmp(key, "pwm_gpio") == 0) {
		int v = str2int(value);
		if (v > 30 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_gpio [%d], "
				"set to default 4", v);
			v = 4;
		}
		ESP_LOGI(TAG, "set config pwm_gpio %d", v);
		config->pwm->gpio = v;
		return 0;
	}
	if (strcmp(key, "pwm_duty") == 0) {
		int v = str2int(value);
		if (v > 254 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_duty [%d], "
				"set to default 100", v);
			v = 100;
		}
		ESP_LOGI(TAG, "set config pwm_duty %d", v);
		config->pwm->duty = v;
		return 0;
	}
	if (strcmp(key, "wifi_ssid") == 0) {
		char *buffer = str_clone(value);
		if (buffer == NULL) {
			ESP_LOGE(TAG, "wifi_ssid failed: malloc failed");
			return ESP_FAIL;
		}
		if (config->wifi->ssid != NULL) {
			free(config->wifi->ssid);
			config->wifi->ssid = NULL;
		}
		config->wifi->ssid = buffer;
		ESP_LOGI(TAG, "set config wifi_ssid %s", buffer);
		return 0;
	}
	if (strcmp(key, "wifi_password") == 0) {
		char *buffer = str_clone(value);
		if (buffer == NULL) {
			ESP_LOGE(TAG, "wifi_password failed: malloc failed");
			return ESP_FAIL;
		}
		if (config->wifi->password != NULL) {
			free(config->wifi->password);
			config->wifi->password = NULL;
		}
		config->wifi->password = buffer;
		ESP_LOGI(TAG, "set config wifi_password %s", buffer);
		return 0;
	}
	if (strcmp(key, "wifi_channel") == 0) {
		int v = str2int(value);
		if (v > 11 || v < 0) {
			ESP_LOGE(TAG, "invalid wifi_channel [%d], "
				"set to default 1", v);
			v = 1;
		}
		ESP_LOGI(TAG, "set config wifi_channel %d", v);
		config->wifi->channel = v;
		return 0;
	}
	if (strcmp(key, "dhcps_ip") == 0) {
		esp_ip4_addr_t ip = str2ipv4(value);
		if (ip.addr == 0 || (ip.addr & 0xff0000ff) == 0) {
			ip.addr = 0x010A0A0A; // 10.10.10.1
			ESP_LOGE(TAG, "invalid dhcps_ip [%s], "
				"set to default: 0x%8X, "IPSTR,
				value, (unsigned int) ip.addr, IP2STR(&ip));
		}
		ESP_LOGI(TAG, "set config dhcps_ip [0x%8X] "IPSTR,
			(unsigned int) ip.addr,  IP2STR(&ip));
		config->dhcps->ip = ip;
		return 0;
	}
	if (strcmp(key, "dhcps_netmask") == 0) {
		esp_ip4_addr_t ip = str2ipv4(value);
		if (ip.addr == 0 || (ip.addr & 0x000000ff) == 0 ||
			(ip.addr & 0x0f000000) > 0) {
			ip.addr = 0x00ffffff; // 255.255.255.0
			ESP_LOGE(TAG, "invalid dhcps_netmask [%s], "
				"set to default "IPSTR, value, IP2STR(&ip));
		}
		ESP_LOGI(TAG, "set config dhcps_netmask [0x%8X] %s",
			(unsigned int) ip.addr, value);
		config->dhcps->netmask = ip;
		return 0;
	}
	if (strcmp(key, "dhcps_as_router") == 0) {
		int v = str2int(value);
		if (v < 0 || v > 1) {
			ESP_LOGE(TAG, "invalid dhcps_as_router [%d], "
				"set to default 0", v);
			v = 1;
		}
		ESP_LOGI(TAG, "set config dhcps_as_router %d", v);
		config->dhcps->as_router = v;
		return 0;
	}
	ESP_LOGE(TAG, "config_set_key_value: unrecognized key [%s] value [%s]",
				key, value);
	return 0;
}

bool is_valid_config_value(char c)
{
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
