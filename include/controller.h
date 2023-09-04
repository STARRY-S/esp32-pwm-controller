#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <esp_err.h>

#define DEFAULT_DUTY_CONFIG "/spiffs/config/duty.txt"

void init_controller(uint8_t pwm_gpio, uint32_t frequency);
esp_err_t controller_set_pwm_duty(uint8_t duty);
esp_err_t controller_get_pwm_duty(uint8_t *val);

#endif