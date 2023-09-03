#include "wifi.h"

#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_err.h>
#include <esp_log.h>

#define TAG "WIFI"

static void wifi_event_handler(
	void* arg,
	esp_event_base_t event_base,
	int32_t event_id,
	void* event_data
) {
	switch (event_id) {
	case WIFI_EVENT_AP_STACONNECTED:
	{
		wifi_event_ap_staconnected_t *event =
			(wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGD(TAG, "station "MACSTR" join, AID=%d",
			MAC2STR(event->mac), event->aid);
		break;
	}
	case WIFI_EVENT_AP_STADISCONNECTED:
	{
		wifi_event_ap_stadisconnected_t* event =
			(wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGD(TAG, "station "MACSTR" leave, AID=%d",
			MAC2STR(event->mac), event->aid);
		break;
	}
	default:
		break;
	}
}

void init_wifi_softap(
	const char *ssid,
	uint8_t channel,
	const char *password,
	uint8_t max_connection
) {
	ESP_LOGI(TAG, "Start setting up WIFI");

	esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(
		esp_event_handler_instance_register(
			WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&wifi_event_handler,
			NULL,
			NULL
		)
	);

	wifi_config_t config = {
		.ap = {
			.ssid_len = strlen(ssid),
			.channel = channel,
			.max_connection = max_connection,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.required = true,
			},
		},
	};
	memset(config.ap.ssid, 0, sizeof(config.ap.ssid));
	memset(config.ap.password, 0, sizeof(config.ap.password));
	memcpy(config.ap.ssid, ssid, strlen(ssid));
	memcpy(config.ap.password, password, strlen(password));
	if (strlen(password) == 0) {
		config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "Finished to init WIFI:\nSSID:%s password:%s channel:%u",
		ssid, password, channel);
}