#ifndef PWM_H
#define PWM_H

#include <esp_err.h>

void init_pwm(int gpio, int channel, int timer, uint32_t frequency);
esp_err_t pwm_set_duty(int channel, uint32_t duty);

#endif