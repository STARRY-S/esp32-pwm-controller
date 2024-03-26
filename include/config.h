#ifndef CONDIF_H
#define CONFIG_H

#include <esp_netif.h>

/**
 * @brief CONFIG_FILE defines the default config file path.
 */
#define CONFIG_FILE "/spiffs/config/config.cfg"

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
 * @brief Controller configuration
 *
 */
struct config {
	struct pwm_config *pwm_fan; // Fan speed configuration
	struct pwm_config *pwm_mos; // Fan power switch (or LED) configuration
	struct wifi_config *wifi;   // WIFI configuration
	struct dhcps_config *dhcps; // DHCP server configuration
};

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
 * @return esp_err_t
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
 * @brief config_set_key_value updates the config by key & value.
 *
 * @param config
 * @param key
 * @param value
 * @return esp_err_t ESP_FAIL if the value of the key is invalid.
 */
esp_err_t config_set_key_value(
	struct config *config,
	const char *key,
	const char *value
);

/**
 * @brief release_config release config allocated memory.
 * The config pointer will be set to NULL after release.
 *
 * @param p pointer points to the config pointer.
 */
void release_config(struct config **p);

#endif // CONDIF_H
