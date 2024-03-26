#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "server.h"
#include "wifi.h"
#include "storage.h"
#include "pwm.h"
#include "controller.h"
#include "config.h"
#include "utils.h"

#define TAG "MAIN"

void app_main()
{
	// Add some timeout before init esp32.
	sleep(1);
	printf("\n");

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(init_storage());

	while (true) {
		ESP_LOGI(TAG, "----------------------------");
		struct config *config = new_config_by_load_file();
		int ret = save_config_file(config);
		if (ret != 0) {
			ESP_LOGE(TAG, "failed to save config: %d", ret);
		}
		release_config(&config);
		if (config != NULL) {
			ESP_LOGE(TAG, "release_config failed");
		}
		sleep(1);
	}
}
