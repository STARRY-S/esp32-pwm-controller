#include <string.h>
#include <esp_log.h>
#include <stdbool.h>

#include "controller.h"
#include "storage.h"
#include "pwm.h"
#include "config.h"

#define TAG "CONTROLLER"
#define PWM_DEFAULT_CHANNEL 0

struct controller {
	struct config *config;
};

/**
 * @brief global controller.
 */
static struct controller controller = {
	.config = NULL,
};

bool controller_initialized()
{
	if (!is_valid_config(controller.config)) {
		return false;
	}
	return true;
}

esp_err_t init_controller()
{
	if (!controller_initialized()) {
		// re-initialize controller config.
		release_config(&controller.config);
	}
	memset(&controller, 0, sizeof(struct controller));

	struct config *config = new_config_by_load_file();
	if (config == NULL) {
		ESP_LOGE(TAG, "init_controller failed");
		return ESP_FAIL;
	}
	controller.config = config;

	return ESP_OK;
}

esp_err_t controller_set_pwm_fan_duty(uint8_t duty)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	char buff[12] = {0};
	sprintf(buff, "%u", duty);
	int ret = config_set_value(
		controller.config, CONFIG_KEY_PWM_FAN_DUTY, buff);
	if (ret != 0) {
		ESP_LOGE(TAG, "failed to set config pwm_fan duty: %d", ret);
		return ret;
	}
	ESP_LOGI(TAG, "set PWM duty: %u", duty);
	return ESP_OK;
}

esp_err_t controller_get_pwm_fan_duty(uint8_t *val)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	uint32_t v = 0;
	int ret = config_get_value(
		controller.config, CONFIG_KEY_PWM_FAN_DUTY, &v, sizeof(v));
	if (ret != 0) {
		ESP_LOGE(TAG, "failed to get config pwm_fan duty: %d", ret);
		return ret;
	}
	*val = (uint8_t) v;
	ESP_LOGI(TAG, "get PWM duty: %u", *val);
	return ESP_OK;
}
