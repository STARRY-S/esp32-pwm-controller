#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include "pwm.h"

#define TAG "PWM"

void init_pwm(int gpio, int channel, int timer, uint32_t frequency)
{
	ESP_LOGI(TAG, "start init pwm gpio [%d], channel [%d], timer [%d]",
			gpio, channel, timer);
	ledc_timer_config_t ledc_timer = {
		.speed_mode       = LEDC_LOW_SPEED_MODE,
		.timer_num        = timer,
		.duty_resolution  = LEDC_TIMER_8_BIT,
		.freq_hz          = frequency,
		.clk_cfg          = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	ledc_channel_config_t ledc_channel = {
		.speed_mode     = LEDC_LOW_SPEED_MODE,
		.channel        = channel,
		.timer_sel      = timer,
		.intr_type      = LEDC_INTR_DISABLE,
		.gpio_num       = gpio,
		.duty           = 0,
		.hpoint         = 0
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
	ESP_LOGI(TAG, "pwm init finished");
}

esp_err_t pwm_set_duty(int channel, uint32_t duty)
{
	esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
	if (ret != ESP_OK) {
		return ret;
	}
	return ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}
