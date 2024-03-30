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

void init()
{
	// Add timeout before start esp32.
	sleep(1);
	printf("\n");
}

void app_main()
{
	init();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(init_storage());
	ESP_ERROR_CHECK(init_global_controller());
	ESP_ERROR_CHECK(global_controller_start());

	while (global_controller_poll_events()) {
		continue;
	}
}
