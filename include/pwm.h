#ifndef PWM_H
#define PWM_H

#include <esp_err.h>

/**
 * @brief initialize controller PWM.
 *
 * @param gpio GPIO number
 * @param channel ledc channel number
 * @param timer ledc timer number
 * @param frequency
 * @return esp_err_t
 */
esp_err_t init_controller_pwm(
	int gpio, int channel, int timer, uint32_t frequency
);

/**
 * @brief set PWM duty (0-255) by ledc channel.
 *
 * @param channel
 * @param duty
 * @return esp_err_t
 */
esp_err_t controller_pwm_set_duty(int channel, uint32_t duty);

#endif
