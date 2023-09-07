#include <string.h>
#include <esp_log.h>
#include <stdbool.h>

#include "controller.h"
#include "storage.h"
#include "pwm.h"

#define TAG "CONTROLLER"
#define PWM_DEFAULT_CHANNEL 0

struct pwm {
	uint8_t channel;
	uint32_t frequency;
	uint8_t gpio;
	uint8_t duty;
};

struct config {
	uint8_t duty;
};

struct controller {
	struct pwm *pwm;
	struct config *config;
};

static struct controller *controller = NULL;

static esp_err_t init_controller_config(struct config *config)
{
	ESP_LOGI(TAG, "start init controller config");
	char *content = NULL;
	int length = read_file(&content, DEFAULT_DUTY_CONFIG);
	if (content == NULL) {
		write_file(DEFAULT_DUTY_CONFIG, "0");
		return ESP_FAIL;
	}
	int duty = 0;
	for (int i = 0; i < length; i++) {
		if (content[i] > '9' || content[i] < '0') {
			continue;
		}
		duty = duty * 10 + content[i] - '0';
	}
	free(content);

	ESP_LOGI(TAG, "read PWM duty from config: %d", duty);
	config->duty = duty;

	return ESP_OK;
}

bool controller_initialized()
{
	if (!controller) {
		return false;
	}
	if (!controller->config) {
		return false;
	}
	if (!controller->pwm) {
		return false;
	}
	return true;
}

void init_controller(uint8_t pwm_gpio, uint32_t frequency)
{
	if (controller_initialized()) {
		return;
	}

	controller = malloc(sizeof(struct controller));
	memset(controller, 0, sizeof(struct controller));
	controller->pwm = malloc(sizeof(struct pwm));
	memset(controller->pwm, 0, sizeof(struct pwm));
	controller->config = malloc(sizeof(struct config));
	memset(controller->config, 0, sizeof(struct config));

	controller->pwm->channel = PWM_DEFAULT_CHANNEL;
	controller->pwm->gpio = pwm_gpio;
	controller->pwm->frequency = frequency;

	ESP_ERROR_CHECK(init_controller_config(controller->config));

	init_pwm(pwm_gpio, PWM_DEFAULT_CHANNEL, 1, frequency);

	ESP_ERROR_CHECK(
		pwm_set_duty(PWM_DEFAULT_CHANNEL, controller->config->duty)
	);
	controller->pwm->duty = controller->config->duty;

	return;
}

esp_err_t controller_set_pwm_duty(uint8_t duty)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	controller->pwm->duty = duty;
	char buffer[8] = { 0 };
	sprintf(buffer, "%u", duty);
	write_file(DEFAULT_DUTY_CONFIG, buffer);
	int ret = pwm_set_duty(controller->pwm->channel, duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to set PWM duty to %u: %d", duty, ret);
	}
	ESP_LOGI(TAG, "set PWM duty: %u", duty);
	return ret;
}

esp_err_t controller_get_pwm_duty(uint8_t *val)
{
	if (!controller_initialized()) {
		ESP_LOGE(TAG, "controller not initialized");
		return ESP_FAIL;
	}

	*val = controller->pwm->duty;
	ESP_LOGI(TAG, "get PWM duty: %u", *val);
	return ESP_OK;
}
