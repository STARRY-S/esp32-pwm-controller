#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// #include <esp_eth.h>
#include <esp_netif.h>
#include <nvs_flash.h>

#include "server.h"
#include "wifi.h"

#define WIFI_SSID    "ESP32_WIFI"
#define WIFI_PASS    "testpassword123"
#define WIFI_CHANNEL 1
#define MAX_STA_CONN 4

void app_main()
{
	// Setup
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_softap(WIFI_SSID, WIFI_CHANNEL, WIFI_PASS, MAX_STA_CONN);

	while (1) {
		sleep(1);
	}
}