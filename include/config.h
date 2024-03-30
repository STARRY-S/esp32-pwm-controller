#ifndef CONFIG_H
#define CONFIG_H

#include <esp_netif.h>

/**
 * @brief config key definitions.
 */

#define CONFIG_KEY_PWM_FAN_CHANNEL 	"pwm_fan_channel"
#define CONFIG_KEY_PWM_FAN_FREQUENCY 	"pwm_fan_frequency"
#define CONFIG_KEY_PWM_FAN_GPIO 	"pwm_fan_gpio"
#define CONFIG_KEY_PWM_FAN_DUTY 	"pwm_fan_duty"
#define CONFIG_KEY_PWM_MOS_CHANNEL 	"pwm_mos_channel"
#define CONFIG_KEY_PWM_MOS_FREQUENCY 	"pwm_mos_frequency"
#define CONFIG_KEY_PWM_MOS_GPIO 	"pwm_mos_gpio"
#define CONFIG_KEY_PWM_MOS_DUTY 	"pwm_mos_duty"
#define CONFIG_KEY_WIFI_SSID 		"wifi_ssid"
#define CONFIG_KEY_WIFI_PASSWORD 	"wifi_password"
#define CONFIG_KEY_WIFI_CHANNEL 	"wifi_channel"
#define CONFIG_KEY_DHCPS_IP 		"dhcps_ip"
#define CONFIG_KEY_DHCPS_NETMASK 	"dhcps_netmask"
#define CONFIG_KEY_DHCPS_AS_ROUTER 	"dhcps_as_router"

/**
 * @brief PWM configuration.
 */
struct pwm_config {
	uint8_t channel;    // PWM channel
	uint32_t frequency; // PWM frequency
	uint8_t gpio;       // GPIO pin
	uint8_t duty;       // PWM duty (0-255)
};

/**
 * @brief WIFI configuration
 */
struct wifi_config {
	char* ssid;      // Wifi SSID
	char* password;  // Wifi password
	uint8_t channel; // Wifi channel (1-11)
};

/**
 * @brief DHCP Server configuration
 */
struct dhcps_config {
	esp_ip4_addr_t ip;      // Interface IPv4 addr
	esp_ip4_addr_t netmask; // Interface netmask (default 255.255.255.0)

	/**
	 * @brief If the as_router is 0, the iPhone will still use cellular
	 * data when connected to this WIFI.
	 * If the as_router is set to 1, the iPhone will not use cellular
	 * data when connected to this WIFI.
	 * Only available for iPhone devices.
	 */
	uint8_t as_router;
};

/**
 * @brief config struct object.
 * Use `new_config_by_load_file` to create config obj from config file.
 * Use `config_set_value` to set value by key.
 * Use `config_get_value` to get value by key.
 * Use `save_config_file` to save the config obj to config file.
 * Use `release_config` to release the config obj.
 */
struct config {
	struct pwm_config *pwm_fan; // Fan speed configuration
	struct pwm_config *pwm_mos; // Fan power switch (or LED) configuration
	struct wifi_config *wifi;   // WIFI configuration
	struct dhcps_config *dhcps; // DHCP server configuration
};

/**
 * @brief CONFIG_FILE defines the default config file path.
 */
#define CONFIG_FILE "/spiffs/config/config.cfg"

/**
 * @brief CONFIG_JSON_TEMPLATE_FILE defines the default config json
 * marshal template
 */
#define CONFIG_JSON_TEMPLATE_FILE "/spiffs/config/config_template.json"

/**
 * @brief new_config_by_load_file builds config struct object from the default
 * config file, need to release by `release_config` manually after usage.
 *
 * @return struct config*
 */
struct config* new_config_by_load_file();

/**
 * @brief save_config_file saves config into the default config file.
 *
 * @param config
 * @return ESP_OK if succeed.
 * @return ESP_FAIL if failed.
 */
esp_err_t save_config_file(struct config *config);

/**
 * @brief is_valid_config detects whether the config is initialized and
 * its content is valid.
 *
 * @param config
 * @return bool
 */
bool is_valid_config(struct config *config);

/**
 * @brief config_set_value updates the config by key & value.
 *
 * @param config pointer points to the struct config obj.
 * @param key CONFIG_KEY_* key
 * @param value value string
 * @return ESP_OK if succeed.
 * @return ESP_FAIL if the value of the key is invalid.
 */
esp_err_t config_set_value(
	struct config *config,
	const char *key,
	const char *value
);

/**
 * @brief config_get_value gets the config by key.
 *
 * @param config pointer points to the struct config obj.
 * @param key CONFIG_KEY_* key
 * @param value pointer points to the value memory.
 * the data type of the value should only be uint32_t* or char*.
 * @param size memory size of the value pointer, should >= 4 (32bit).
 * @return ESP_OK if succeed.
 * @return ESP_FAIL if the value of the key is invalid.
 */
esp_err_t config_get_value(
	struct config *config,
	const char *key,
	void* value,
	int size
);

/**
 * @brief
 *
 * @param config
 * @param data
 * @return esp_err_t
 */
esp_err_t config_marshal_json(struct config *config, char* data);

/**
 * @brief release_config release config allocated memory.
 * The config pointer will be set to NULL after release.
 *
 * @param p pointer points to the config pointer.
 */
void release_config(struct config **p);

#endif // CONFIG_H
