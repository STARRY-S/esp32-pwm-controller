#include <string.h>
#include <esp_log.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_netif.h>

#include "config.h"
#include "storage.h"
#include "utils.h"

#define TAG "CONFIG"
#define BUFF_SIZE (128 * sizeof(char))

/**
 * @brief new_config_by_config_file_data will parse the config file data,
 * if the config value in the data is invalid, it will be reset to the
 * default value.
 * This function will ensure the returned config is a valid config object.
 *
 * @param data config file data
 * @param length config file data length
 * @return const struct config*
 */
struct config* new_config_by_config_file_data(
	const char *data,
	int length
);

bool is_valid_config_value(char c);

struct config* new_config_by_load_file()
{
	char *content = NULL;
	int ret = read_file(&content, CONFIG_FILE);
	if (content == NULL || ret <= 0) {
		ESP_LOGE(TAG, "failed to open config %s: %d", CONFIG_FILE, ret);
		return NULL;
	}
	struct config *config = new_config_by_config_file_data(content, ret);
	free(content);
	content = NULL;
	return config;
}

esp_err_t save_config_file(struct config *config)
{
	if (!is_valid_config(config)) {
		ESP_LOGE(TAG, "save_config_file failed: invalid config");
		return ESP_FAIL;
	}
	char *buffer = malloc(BUFF_SIZE * 128);
	memset(buffer, 0, BUFF_SIZE*128);

	static const char * config_template =
		"pwm_fan_channel=%u\n"
		"pwm_fan_frequency=%u\n"
		"pwm_fan_gpio=%u\n"
		"pwm_fan_duty=%u\n"
		"pwm_mos_channel=%u\n"
		"pwm_mos_frequency=%u\n"
		"pwm_mos_gpio=%u\n"
		"pwm_mos_duty=%u\n"
		"wifi_ssid=%s\n"
		"wifi_password=%s\n"
		"wifi_channel=%u\n"
		"dhcps_ip="IPSTR"\n"
		"dhcps_netmask="IPSTR"\n"
		"dhcps_as_router=%u\n"
		;

	sprintf(buffer,
		config_template,
		config->pwm_fan->channel,
		(unsigned int) config->pwm_fan->frequency,
		config->pwm_fan->gpio,
		config->pwm_fan->duty,
		config->pwm_mos->channel,
		(unsigned int) config->pwm_mos->frequency,
		config->pwm_mos->gpio,
		config->pwm_mos->duty,
		config->wifi->ssid,
		config->wifi->password,
		config->wifi->channel,
		IP2STR(&config->dhcps->ip),
		IP2STR(&config->dhcps->netmask),
		config->dhcps->as_router
	);

	ESP_LOGI(TAG, "save_config_file:\n%s", buffer);
	int ret = write_file(CONFIG_FILE, buffer);
	if (ret <= 0) {
		ESP_LOGE(TAG, "save_config_file: write_file failed: %d", ret);
	}
	free(buffer);

	return ESP_OK;
}

bool is_valid_config(struct config *config)
{
	if (!config) {
		ESP_LOGE(TAG, "is_valid_config: config is NULL");
		return false;
	}
	if (!config->pwm_fan) {
		ESP_LOGE(TAG, "is_valid_config: pwm_fan is NULL");
		return false;
	}
	if (config->pwm_fan->channel > 30) {
		ESP_LOGE(TAG, "is_valid_config: pwm_fan->channel: "
			"invalid value");
		return false;
	}
	if (config->pwm_fan->frequency > 100000 ||
		config->pwm_fan->frequency < 1000) {
		ESP_LOGE(TAG, "is_valid_config: pwm_fan->frequency: "
			"invalid value");
		return false;
	}
	if (config->pwm_fan->gpio > 30) {
		ESP_LOGE(TAG, "is_valid_config: pwm_fan->gpio: "
			"invalid value");
		return false;
	}
	// config->pwm_fan->duty will always been 0-255.

	if (!config->pwm_mos) {
		ESP_LOGE(TAG, "is_valid_config: pwm_mos is NULL");
		return false;
	}
	if (config->pwm_mos->channel > 30) {
		ESP_LOGE(TAG, "is_valid_config: pwm_mos->channel: "
			"invalid value");
		return false;
	}
	if (config->pwm_mos->frequency > 100000 ||
		config->pwm_mos->frequency < 1000) {
		ESP_LOGE(TAG, "is_valid_config: pwm_mos->frequency: "
			"invalid value");
		return false;
	}
	if (config->pwm_mos->gpio > 30) {
		ESP_LOGE(TAG, "is_valid_config: pwm_mos->gpio: "
			"invalid value");
		return false;
	}
	// config->pwm_mos->duty will always been 0-255.

	if (!config->wifi) {
		ESP_LOGE(TAG, "is_valid_config: wifi is NULL");
		return false;
	}
	if (!config->wifi->password) {
		ESP_LOGE(TAG, "is_valid_config: wifi->password is NULL");
		return false;
	}
	const char* p = config->wifi->password;
	int len = strlen(p);
	if (len > 30 || len < 8) {
		ESP_LOGE(TAG, "is_valid_config: wifi->password: "
			"invalid password length");
	}
	for (int i = 0; p[i] != '\0'; i++) {
		if (is_valid_config_value(p[i])) {
			continue;
		}
		ESP_LOGE(TAG, "is_valid_config: wifi->password: "
			"invalid char [%c]", p[i]);
		return false;
	}
	if (!config->wifi->ssid) {
		ESP_LOGE(TAG, "is_valid_config: wifi->ssid is NULL");
		return false;
	}
	p = config->wifi->ssid;
	len = strlen(p);
	if (len > 30 || len < 1) {
		ESP_LOGE(TAG, "is_valid_config: wifi->ssid: "
			"invalid ssid length");
	}
	for (int i = 0; p[i] != '\0'; i++) {
		if (is_valid_config_value(p[i])) {
			continue;
		}
		ESP_LOGE(TAG, "is_valid_config: wifi->ssid: "
			"invalid char [%c]", p[i]);
		return false;
	}
	if (!config->dhcps) {
		ESP_LOGE(TAG, "is_valid_config: dhcps is NULL");
		return false;
	}
	if ((config->dhcps->ip.addr & 0x000000ff) == 0) { // 255.0.0.0
		ESP_LOGE(TAG, "is_valid_config: dhcps->ip: "
			"invalid value");
		return false;
	}
	if ((config->dhcps->netmask.addr & 0x000000ff) == 0 || // 255.0.0.0
		(config->dhcps->netmask.addr & 0xff000000) > 0) { // 0.0.0.255
		ESP_LOGE(TAG, "is_valid_config: dhcps->ip: "
			"invalid value");
		return false;
	}
	// as_router is used as a bool value.

	return true;
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

struct config* new_config_default_value()
{
	struct config *config = malloc(sizeof(struct config));
	if (config == NULL) {
		ESP_LOGE(TAG, "new_config_default_value failed: malloc fail");
		return NULL;
	}
	config->pwm_fan = malloc(sizeof(struct pwm_config));
	config->pwm_mos = malloc(sizeof(struct pwm_config));
	config->wifi = malloc(sizeof(struct wifi_config));
	config->dhcps = malloc(sizeof(struct dhcps_config));
	if (!config->pwm_fan || !config->pwm_mos || !config->wifi ||
		!config->dhcps) {
		release_config(&config);
		ESP_LOGE(TAG, "new_config_default_value failed: malloc fail");
		return NULL;
	}

	// Init config with default values.
	config->pwm_fan->channel = 0;
	config->pwm_fan->frequency = 25000;
	config->pwm_fan->gpio = 4;
	config->pwm_fan->duty = 100;

	config->pwm_mos->channel = 1;
	config->pwm_mos->frequency = 25000;
	config->pwm_mos->gpio = 8;
	config->pwm_mos->duty = 255;

	config->wifi->ssid = str_clone("PWM_FAN_CONTROLLER");
	config->wifi->password = str_clone("testpassword123");
	config->wifi->channel = 1;
	config->dhcps->ip.addr = 0x010A0A0A; // 10.10.10.1
	config->dhcps->netmask.addr = 0x00ffffff; // 255.255.255.0
	config->dhcps->as_router = 0;
	return config;
}

struct config* new_config_by_config_file_data(
	const char *content, int length
) {
	if (content == NULL) {
		ESP_LOGE(TAG, "new_config_by_config_file_data failed: "
			"content is NULL");
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
		ESP_LOGE(TAG, "new_config_by_config_file_data failed: "
			"malloc failed");
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
				ESP_LOGE(TAG,
					"new_config_by_config_file_data: "
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
			ESP_LOGD(TAG, "read key [%s] value [%s]", key, value);
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

esp_err_t config_set_key_value(
	struct config *config, const char *key, const char *value
){
	if (config == NULL || key == NULL || value == NULL) {
		ESP_LOGE(TAG, "config_set_key_value failed: config NULL ptr");
		return ESP_FAIL;
	}
	if (!config->dhcps || !config->pwm_fan || !config->wifi) {
		ESP_LOGE(TAG, "config_set_key_value failed: config NULL ptr");
		return ESP_FAIL;
	}
	if (strcmp(key, "pwm_fan_channel") == 0) {
		int v = str2int(value);
		if (v > 5) {
			ESP_LOGE(TAG, "invalid PWM channel [%d], "
				"set to default 0", v);
			v = 0;
		}
		ESP_LOGD(TAG, "set config pwm_fan_channel %d", v);
		config->pwm_fan->channel = v;
		return 0;
	}
	if (strcmp(key, "pwm_fan_frequency") == 0) {
		int v = str2int(value);
		if (v > 100000 || v < 1000) {
			ESP_LOGE(TAG, "invalid PWM frequency [%d], "
				"set to default 25000", v);
			v = 25000;
		}
		ESP_LOGD(TAG, "set config pwm_fan_frequency %d", v);
		config->pwm_fan->frequency = v;
		return 0;
	}
	if (strcmp(key, "pwm_fan_gpio") == 0) {
		int v = str2int(value);
		if (v > 30 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_fan_gpio [%d], "
				"set to default 4", v);
			v = 4;
		}
		ESP_LOGD(TAG, "set config pwm_fan_gpio %d", v);
		config->pwm_fan->gpio = v;
		return 0;
	}
	if (strcmp(key, "pwm_fan_duty") == 0) {
		int v = str2int(value);
		if (v > 254 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_fan_duty [%d], "
				"set to default 100", v);
			v = 100;
		}
		ESP_LOGD(TAG, "set config pwm_fan_duty %d", v);
		config->pwm_fan->duty = v;
		return 0;
	}
	if (strcmp(key, "pwm_mos_channel") == 0) {
		int v = str2int(value);
		if (v > 5) {
			ESP_LOGE(TAG, "invalid PWM mos channel [%d], "
				"set to default 1", v);
			v = 1;
		}
		ESP_LOGD(TAG, "set config pwm_mos_channel %d", v);
		config->pwm_mos->channel = v;
		return 0;
	}
	if (strcmp(key, "pwm_mos_frequency") == 0) {
		int v = str2int(value);
		if (v > 100000 || v < 1000) {
			ESP_LOGE(TAG, "invalid PWM frequency [%d], "
				"set to default 25000", v);
			v = 25000;
		}
		ESP_LOGD(TAG, "set config pwm_mos_frequency %d", v);
		config->pwm_mos->frequency = v;
		return 0;
	}
	if (strcmp(key, "pwm_mos_gpio") == 0) {
		int v = str2int(value);
		if (v > 30 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_mos_gpio [%d], "
				"set to default 8", v);
			v = 8;
		}
		ESP_LOGD(TAG, "set config pwm_mos_gpio %d", v);
		config->pwm_mos->gpio = v;
		return 0;
	}
	if (strcmp(key, "pwm_mos_duty") == 0) {
		int v = str2int(value);
		if (v > 255 || v < 0) {
			ESP_LOGE(TAG, "invalid pwm_mos_duty [%d], "
				"set to default 255", v);
			v = 255;
		}
		ESP_LOGD(TAG, "set config pwm_mos_duty %d", v);
		config->pwm_mos->duty = v;
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
		ESP_LOGD(TAG, "set config wifi_ssid %s", buffer);
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
		ESP_LOGD(TAG, "set config wifi_password %s", buffer);
		return 0;
	}
	if (strcmp(key, "wifi_channel") == 0) {
		int v = str2int(value);
		if (v > 11 || v < 0) {
			ESP_LOGE(TAG, "invalid wifi_channel [%d], "
				"set to default 1", v);
			v = 1;
		}
		ESP_LOGD(TAG, "set config wifi_channel %d", v);
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
		ESP_LOGD(TAG, "set config dhcps_ip [0x%8X] "IPSTR,
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
		ESP_LOGD(TAG, "set config dhcps_netmask [0x%8X] %s",
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
		ESP_LOGD(TAG, "set config dhcps_as_router %d", v);
		config->dhcps->as_router = v;
		return 0;
	}

	ESP_LOGE(TAG, "config_set_key_value: unrecognized key [%s]", key);
	return ESP_FAIL;
}

void release_config(struct config **p) {
	if (p == NULL) {
		return;
	}
	struct config *config = *p;
	if (config == NULL) {
		return;
	}
	if (config->dhcps != NULL) {
		free(config->dhcps);
		config->dhcps = NULL;
	}
	if (config->pwm_fan != NULL) {
		free(config->pwm_fan);
		config->pwm_fan = NULL;
	}
	if (config->pwm_mos != NULL) {
		free(config->pwm_mos);
		config->pwm_mos = NULL;
	}
	if (config->wifi != NULL) {
		free(config->wifi->password);
		free(config->wifi->ssid);
		config->wifi->password = config->wifi->ssid = NULL;
		free(config->wifi);
		config->wifi = NULL;
	}
	free(config);
	*p = NULL;
}
