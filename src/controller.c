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

static struct controller *controller = NULL;

bool controller_initialized()
{
	if (!controller) {
		return false;
	}
	if (!controller->config) {
		return false;
	}
	return true;
}

esp_err_t init_controller_config()
{
	if (controller != NULL) {
		release_config(&controller->config);
		free(controller);
		controller = NULL;
	}

	controller = malloc(sizeof(struct controller));
	if (controller == NULL) {
		ESP_LOGE(TAG, "failed to init controller: malloc failed");
		return ESP_FAIL;
	}
	memset(controller, 0, sizeof(struct controller));

	struct config *config = new_config_by_load_file();
	if (config == NULL) {
		ESP_LOGE(TAG, "init_controller_config failed");
		return ESP_FAIL;
	}
	controller->config = config;

	return ESP_OK;
}

esp_err_t controller_set_pwm_fan_duty(uint8_t duty)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	controller->config->pwm_fan->duty = duty;
	ESP_LOGI(TAG, "set PWM duty: %u", duty);
	return ESP_OK;
}

esp_err_t controller_get_pwm_fan_duty(uint8_t *val)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	*val = controller->config->pwm_fan->duty;
	ESP_LOGI(TAG, "get PWM duty: %u", *val);
	return ESP_OK;
}
