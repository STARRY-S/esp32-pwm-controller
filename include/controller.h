#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <esp_err.h>

/**
 * @brief init_controller_config initializes the controller struct object.
 *
 * @return esp_err_t
 */
esp_err_t init_controller_config();

/**
 * @brief
 *
 * @param duty
 * @return esp_err_t
 */
esp_err_t controller_set_pwm_duty(uint8_t duty);

esp_err_t controller_get_pwm_duty(uint8_t *val);

#endif
