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
static int default_controller_apply_pwm_duty(struct controller*);

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
	controller->apply_pwm_duty = default_controller_apply_pwm_duty;

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

	// Start web server.
	int ret = 0;
	if ((ret = controller->start_server(controller)) != 0) {
		ESP_LOGE(TAG, "global_controller_start: "
			"failed to start web server: [%d]", ret);
		return ret;
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

bool global_controller_main_loop()
{
	if (!is_global_controller_running()) {
		return false;
	}
	// sleep 100000 us (0.1s, 100ms).
	usleep(50000);

	return true;
}

esp_err_t global_controller_apply_pwm_duty()
{
	if (!controller_initialized(controller)) {
		ESP_LOGE(TAG,
			"global_controller_apply_pwm_duty: not initialized");
		return ESP_FAIL;
	}
	return controller->apply_pwm_duty(controller);
}

esp_err_t global_controller_update_config(const char* k, const char* v)
{
	if (!controller_initialized(controller)) {
		ESP_LOGE(TAG,
			"global_controller_apply_pwm_duty: not initialized");
		return ESP_FAIL;
	}
	return controller->update_config(controller, k, v);
}

esp_err_t global_controller_config_marshal_json(char *data)
{
	return config_marshal_json(controller->config, data);
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

	int ret = 0;

	// start wifi soft AP & dhcp server.
	ESP_LOGI(TAG, "start default wifi soft AP");
	if ((ret = init_controller_wifi_softap(controller->config)) != 0) {
		return ret;
	}

	ESP_LOGI(TAG, "start default http web server");
	ret = start_default_http_server(&c->server_handle, c->config);
	if (ret != 0) {
		ESP_LOGE(TAG, "start_default_http_server failed: [%d]", ret);
		return ret;
	}

	// init PWM for fan.
	ret = init_controller_pwm(
		controller->config->pwm_fan->gpio,
		controller->config->pwm_fan->channel,
		DEFAULT_PWM_FAN_TIMER,
		controller->config->pwm_fan->frequency);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "init_controller_pwm for pwm_fan failed: "
			"[%d]", ret);
		return ret;
	}
	ret = controller_pwm_set_duty(
		controller->config->pwm_fan->channel,
		controller->config->pwm_fan->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_pwm_set_duty for pwm_fan failed: "
			"[%d]", ret);
		return ret;
	}

	// init PWM for MOSFET (or LED).
	ret = init_controller_pwm(
		controller->config->pwm_mos->gpio,
		controller->config->pwm_mos->channel,
		DEFAULT_PWM_MOS_TIMER,
		controller->config->pwm_mos->frequency);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "init_controller_pwm for pwm_mos failed: "
			"[%d]", ret);
		return ret;
	}
	ret = controller_pwm_set_duty(
		controller->config->pwm_mos->channel,
		controller->config->pwm_mos->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_pwm_set_duty for pwm_mos failed: "
			"[%d]", ret);
		return ret;
	}

	return ESP_OK;
}

static int default_controller_stop(struct controller* c)
{
	if (!c || !c->server_handle) {
		// server already stopped.
		return ESP_OK;
	}
	ESP_LOGI(TAG, "controller server will restart");
	int ret = 0;
	if ((ret = save_config_file(c->config)) != 0) {
		ESP_LOGW(TAG, "default_controller_stop: "
			"save_config_file: [%d]", ret);
	}
	if ((ret = stop_default_http_server(c->server_handle)) != 0) {
		ESP_LOGW(TAG, "default_controller_stop: "
			"failed to stop http server: [%d]", ret);
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

static int default_controller_apply_pwm_duty(struct controller* c) {
	// Update fan PWM duty & MOSFET duty without reboot.
	int ret = 0;
	ret = controller_pwm_set_duty(
		c->config->pwm_fan->channel, c->config->pwm_fan->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_pwm_set_duty for pwm_fan failed: "
			"[%d]", ret);
		return ret;
	}
	ret = controller_pwm_set_duty(
		c->config->pwm_mos->channel, c->config->pwm_mos->duty);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "controller_pwm_set_duty for pwm_mos failed: "
			"[%d]", ret);
		return ret;
	}
	return 0;
}
