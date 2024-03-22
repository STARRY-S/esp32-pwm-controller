#include <string.h>
#include <stdbool.h>
#include <esp_netif.h>

#include "utils.h"

int str2int(const char *const value)
{
	int length = strlen(value);
	int num = 0;
	bool negative = false;
	for (int i = 0; i < length; i++) {
		if (value[i] == '-') {
			negative = -negative;
		}
		if (value[i] > '9' || value[i] < '0') {
			continue;
		}
		num = num * 10 + value[i] - '0';
	}
	return num;
}

char* int2str(int value)
{
	char *buffer = malloc(sizeof(char) * 11);
	memset(buffer, 0, sizeof(char) * 11);
	int i = 0;
	if (value < 0) {
		value = -value;
		buffer[0] = '-';
		++i;
	}
	for (; value > 0 && i < 11; i++) {
		buffer[i] = '0' + value % 10;
		value /= 10;
	}
	return buffer;
}

char* str_clone(const char *const value)
{
	if (value == NULL) {
		return NULL;
	}
	int length = strlen(value);
	char *buffer = malloc((length+1) * sizeof(char));
	memcpy(buffer, value, (length+1) * sizeof(char));
	return buffer;
}

esp_ip4_addr_t str2ipv4(const char *const value)
{
	esp_ip4_addr_t ip;
	ip.addr = 0;

	if (value == NULL) {
		return ip;
	}
	uint8_t num = 0;
	uint32_t addr = 0;
	int length = strlen(value);
	int count = 0;
	for (int i = 0; i < length; i++) {
		if (value[i] == '.') {
			if (num > 255) {
				// Return as a invalid IPv4 address.
				return ip;
			}
			addr += num << (8 * count);
			num = 0;
			count++;
		}
		if (value[i] > '9' || value[i] < '0') {
			continue;
		}
		num = num*10 + value[i] - '0';
	}
	if (count != 3) {
		return ip;
	}
	if (num > 255) {
		return ip;
	}
	addr += num << 24;
	ip.addr = addr;

	return ip;
}
