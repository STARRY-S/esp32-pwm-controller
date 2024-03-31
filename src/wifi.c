#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <lwip/ip_addr.h>

#include "wifi.h"

#define TAG "WIFI"
#define DEFAULT_WIFI_MAX_CONNECTION 4

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
	}
}

esp_err_t init_controller_wifi_softap(struct config *c)
{
	int ret = 0;
	esp_netif_t *wifi_ap = esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ret = esp_wifi_init(&cfg);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_init failed [%d]", ret);
		return ret;
	}
	ret = esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&wifi_event_handler,
		NULL,
		NULL
	);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "esp_event_handler_instance_register [%d]",
			ret);
		return ret;
	}

	wifi_config_t config = {
		.ap = {
			.ssid_len = strlen(c->wifi->ssid),
			.channel = c->wifi->channel,
			.max_connection = DEFAULT_WIFI_MAX_CONNECTION,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.required = true,
			},
		},
	};
	memset(config.ap.ssid, 0, sizeof(config.ap.ssid));
	memset(config.ap.password, 0, sizeof(config.ap.password));
	memcpy(config.ap.ssid, c->wifi->ssid, strlen(c->wifi->ssid));
	memcpy(config.ap.password,
		c->wifi->password, strlen(c->wifi->password));
	if (strlen(c->wifi->password) == 0) {
		config.ap.authmode = WIFI_AUTH_OPEN;
		config.ap.pmf_cfg.required = false;
	}

	if ((ret = esp_wifi_set_mode(WIFI_MODE_AP)) != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_set_mode [%d]", ret);
		return ret;
	}
	if ((ret = esp_wifi_set_config(WIFI_IF_AP, &config)) != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_set_config [%d]", ret);
		return ret;
	}

	// Restart DHCP server to set options.
	if ((ret = esp_netif_dhcps_stop(wifi_ap)) != ESP_OK) {
		if (ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
			ESP_LOGE(TAG, "esp_wifi_start [%d]", ret);
			return ret;
		}
	}

	// If the as_router is 0, it will allow iPhone to use cellular data
	// when connected to this wifi AP.
	ret = esp_netif_dhcps_option(
		wifi_ap,
		ESP_NETIF_OP_SET,
		ESP_NETIF_ROUTER_SOLICITATION_ADDRESS,
		&c->dhcps->as_router,
		sizeof(c->dhcps->as_router)
	);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "esp_netif_dhcps_option [%d]", ret);
		return ret;
	}

        // Set IP address.
        esp_netif_ip_info_t info = {
		.ip = c->dhcps->ip,
		.gw = c->dhcps->ip,
		.netmask = c->dhcps->netmask
	};
	if ((ret = esp_netif_set_ip_info(wifi_ap, &info)) != ESP_OK) {
		ESP_LOGE(TAG, "esp_netif_set_ip_info [%d]", ret);
		return ret;
	}

        // Restart DHCP server to set options.
        if ((ret = esp_netif_dhcps_start(wifi_ap)) != ESP_OK) {
		ESP_LOGE(TAG, "esp_netif_dhcps_start [%d]", ret);
		return ESP_FAIL;
	}

	if ((ret = esp_wifi_start()) != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_start [%d]", ret);
		return ret;
	}
	ESP_LOGI(TAG, "init WIFI: SSID [%s] channel [%u]",
		c->wifi->ssid, c->wifi->channel);

	return ESP_OK;
}
