#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <dirent.h>
#include <esp_spiffs.h>
#include <sys/stat.h>

#include "server.h"
#include "wifi.h"
#include "storage.h"
#include "pwm.h"
#include "controller.h"

#define TAG "MAIN"
#define WIFI_SSID "FURSUIT_HEAD_CONTROL"
#define WIFI_PASS "testpassword123"
#define PWM_GPIO 25
#define PWM_FREQ 25000

void app_main()
{
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	init_wifi_softap(WIFI_SSID, 1, WIFI_PASS, 4);
	init_spiffs_storage();
	init_controller(PWM_GPIO, PWM_FREQ);

	httpd_handle_t *server = start_webserver(80);
	while (server) {
		sleep(5);
	}
}
