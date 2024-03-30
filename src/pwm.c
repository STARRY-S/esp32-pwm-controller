#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include "pwm.h"

#define TAG "PWM"

esp_err_t init_controller_pwm(
	int gpio, int channel, int timer, uint32_t frequency
) {
	int ret = 0;
	ledc_timer_config_t ledc_timer = {
		.speed_mode       = LEDC_LOW_SPEED_MODE,
		.timer_num        = timer,
		.duty_resolution  = LEDC_TIMER_8_BIT,
		.freq_hz          = frequency,
		.clk_cfg          = LEDC_AUTO_CLK
	};
	if ((ret = ledc_timer_config(&ledc_timer)) != ESP_OK) {
		ESP_LOGE(TAG, "init_pwm: ledc_timer_config fail [%d]", ret);
		return ret;
	}

	ledc_channel_config_t ledc_channel = {
		.speed_mode     = LEDC_LOW_SPEED_MODE,
		.channel        = channel,
		.timer_sel      = timer,
		.intr_type      = LEDC_INTR_DISABLE,
		.gpio_num       = gpio,
		.duty           = 0,
		.hpoint         = 0
	};

	if ((ret = ledc_channel_config(&ledc_channel)) != ESP_OK) {
		ESP_LOGE(TAG, "init_pwm: ledc_channel_config fail [%d]", ret);
		return ret;
	}
	ESP_LOGI(TAG, "init pwm gpio [%d], channel [%d], timer [%d]",
			gpio, channel, timer);
	return ESP_OK;
}

esp_err_t controller_pwm_set_duty(int channel, uint32_t duty)
{
	esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
	if (ret != ESP_OK) {
		return ret;
	}
	ESP_LOGI(TAG, "update pwm channel [%d] duty [0x%X]",
		channel, (unsigned) duty);
	return ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}
