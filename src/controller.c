#include <string.h>
#include <esp_log.h>
#include <stdbool.h>
#include <unistd.h>

#include "controller.h"
#include "storage.h"
#include "pwm.h"
#include "config.h"
#include "server.h"
#include "wifi.h"

#define TAG "CONTROLLER"
#define DEFAULT_PWM_FAN_TIMER 0
#define DEFAULT_PWM_MOS_TIMER 1

static int default_controller_start(struct controller*);
static int default_controller_stop(struct controller*);
static int default_controller_save_config(struct controller*);
static int default_controller_update_config(
	struct controller*, const char*, const char *);
static int default_controller_update_pwm_duty(struct controller*);

/**
 * @brief PWM Fan controller struct object.
 */
struct controller {
	struct config *config;
	httpd_handle_t server_handle;

        /**
         * @brief start starts the controller web server.
         *
         * @return ESP_OK if succeed.
         * @return ESP_FAIL if failed.
         */
	int (*start_server)(struct controller*);

        /**
         * @brief stop stops the controller web server.
         *
         * @return ESP_OK if succeed.
         * @return ESP_FAIL if failed.
         */
	int (*stop_server)(struct controller*);

        /**
         * @brief save_config saves the controller config into the default
         * config file.
         *
         * @return ESP_OK if succeed.
         * @return ESP_FAIL if failed.
         */
        int (*save_config)(struct controller*);

        /**
         * @brief update_config updates the config by key and value.
         *
         * @return ESP_OK if succeed.
         * @return ESP_FAIL if failed.
         */
        int (*update_config)(struct controller*, const char* k, const char *v);

        /**
         * @brief apply_config apply the controller config.
	 * NOTE: some config requires reboot to take effect.
         *
         * @return ESP_OK if succeed.
         * @return ESP_FAIL if failed.
         */
        int (*apply_pwm_duty)(struct controller*);
};

/**
 * @brief global private controller.
 */
struct controller *controller = NULL;

esp_err_t init_global_controller()
{
	if (controller_initialized(controller)) {
		// re-initialize controller config if already initialized.
		release_config(&controller->config);
	}
	if (controller == NULL) {
		controller = malloc(sizeof(struct controller));
	}
	memset(controller, 0, sizeof(struct controller));

	ESP_LOGI(TAG, "start init global controller");
	// Load config from default config file.
	struct config *config = new_config_by_load_file();
	if (config == NULL) {
		ESP_LOGE(TAG, "new_controller failed");
		return ESP_FAIL;
	}
	controller->config = config;
	controller->start_server = default_controller_start;
	controller->stop_server = default_controller_stop;
	controller->update_config = default_controller_update_config;
	controller->save_config = default_controller_save_config;
	controller->apply_pwm_duty = default_controller_update_pwm_duty;

	return ESP_OK;
}

struct controller *get_global_controller()
{
	return controller;
}

esp_err_t global_controller_start()
{
	if (!controller_initialized(controller)) {
		ESP_LOGE(TAG, "global_controller_start: not initialized");
		return ESP_FAIL;
	}
	int ret = 0;

	// Start wifi AP & dhcp server.
	if ((ret = init_controller_wifi_softap(controller->config)) != 0) {
		return ret;
	}
	// Start web server.
	if ((ret = controller->start_server(controller)) != 0) {
		ESP_LOGE(TAG, "global_controller_start: "
			"failed to start web server: [%d]", ret);
		return ret;
	}

	// Init PWM for fan.
	ret = init_controller_pwm(
		controller->config->pwm_fan->gpio,
		controller->config->pwm_fan->channel,
		DEFAULT_PWM_FAN_TIMER,
		controller->config->pwm_fan->frequency);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "global_controller_start: "
			"init_controller_pwm for pwm_fan: [%d]", ret);
		return ESP_FAIL;
	}
	ret = controller_pwm_set_duty(
		controller->config->pwm_fan->channel,
		controller->config->pwm_fan->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "global_controller_start: "
			"controller_pwm_set_duty for pwm_fan: [%d]", ret);
		return ESP_FAIL;
	}

	// Init PWM for MOS (LED).
	ret = init_controller_pwm(
		controller->config->pwm_mos->gpio,
		controller->config->pwm_mos->channel,
		DEFAULT_PWM_MOS_TIMER,
		controller->config->pwm_mos->frequency);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "global_controller_start: "
			"init_controller_pwm for pwm_mos: [%d]", ret);
		return ESP_FAIL;
	}
	ret = controller_pwm_set_duty(
		controller->config->pwm_mos->channel,
		controller->config->pwm_mos->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "global_controller_start: "
			"controller_pwm_set_duty for pwm_mos: [%d]", ret);
		return ESP_FAIL;
	}

	return ESP_OK;
}

bool is_global_controller_running()
{
	if (!controller_initialized(controller)) {
		return false;
	}
	return controller->server_handle != NULL;
}

bool global_controller_poll_events()
{
	if (!is_global_controller_running()) {
		return false;
	}
	// sleep 50000 us (0.05s, 50ms).
	usleep(50000);

	return true;
}

esp_err_t global_controller_stop()
{
	if (!controller_initialized(controller)) {
		ESP_LOGE(TAG, "global_controller_stop: not initialized");
		return ESP_FAIL;
	}
	return controller->stop_server(controller);
}

bool controller_initialized(struct controller *c)
{
	if (!c) {
		ESP_LOGD(TAG, "controller_initialized: c is NULL");
		return false;
	}
	if (!is_valid_config(c->config)) {
		ESP_LOGD(TAG, "controller_initialized: config is invalid");
		return false;
	}
	if (!c->start_server || !c->stop_server) {
		ESP_LOGD(TAG, "controller_initialized: start/stop is NULL");
		return false;
	}
	if (!c->save_config || !c->update_config || !c->apply_pwm_duty) {
		ESP_LOGD(TAG, "controller_initialized: "
			"save/update/apply is NULL");
		return false;
	}
	return true;
}

static int default_controller_start(struct controller* c)
{
	if (!controller_initialized(c)) {
		ESP_LOGE(TAG, "default_controller_start: invalid param");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "start default http web server");
	return start_default_http_server(&c->server_handle, c->config);
}

static int default_controller_stop(struct controller* c)
{
	if (!c || !c->server_handle) {
		// server already stopped.
		return ESP_OK;
	}
	int ret = 0;
	if ((ret = stop_default_http_server(c->server_handle)) != 0) {
		return ret;
	}
	c->server_handle = NULL;
	return ESP_OK;
}

static int default_controller_save_config(struct controller* c)
{
	return save_config_file(c->config);
}

static int default_controller_update_config(
	struct controller *c, const char* k, const char *v
) {
	return config_set_value(c->config, k, v);
}

static int default_controller_update_pwm_duty(struct controller*)
{
	// Update PWM fan speed without reboot.
	return 0;
}
