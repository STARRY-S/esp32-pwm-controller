#ifndef WIFI_H
#define WIFI_H

#include <string.h>

void init_wifi_softap(
	const char *ssid,
	uint8_t channel,
	const char *password,
	uint8_t max_connection
);

#endif