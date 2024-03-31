#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <esp_err.h>

/**
 * @brief private controller struct object.
 */
struct controller;

/**
 * @brief detect whether the controller obj is initialized or not.
 *
 * @param c
 * @return bool
 */
bool controller_initialized(struct controller *c);

/**
 * @brief init_global_controller initializes a global controller.
 *
 * @return esp_err_t
 */
esp_err_t init_global_controller();

/**
 * @brief get the global controller object.
 *
 * @return struct controller*
 */
struct controller *get_global_controller();

/**
 * @brief start the global controller.
 *
 * @return esp_err_t
 */
esp_err_t global_controller_start();

/**
 * @brief get if the global controller is running.
 *
 * @return bool
 */
bool is_global_controller_running();

/**
 * @brief global_controller_main_loop is the main loop function.
 *
 * @return true if the controller server is running
 * @return false if the controller server is stopped
 */
bool global_controller_main_loop();

esp_err_t global_controller_apply_pwm_duty();

esp_err_t global_controller_update_config(const char* k, const char* v);

esp_err_t global_controller_config_marshal_json(char *data);

/**
 * @brief stop the global controller.
 * need to reboot to start the server agagin.
 *
 * @return esp_err_t
 */
esp_err_t global_controller_stop();

#endif
