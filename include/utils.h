#ifndef UTILS_H
#define UTILS_H

#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_netif.h>

/**
 * @brief str2int converts a non-negative string to int.
 *
 * @param value
 * @return int
 */
int str2int(const char *const value);

/**
 * @brief int2str converts int to string, need to free manually after use.
 *
 * @return string
 */
char* int2str(int value);

/**
 * @brief str_clone returns a clone of the provided string,
 * need to free manually after use.
 *
 * @param value string
 * @return clone string
 */
char* str_clone(const char *const value);

/**
 * @brief str2ipv4 converts the string to ipv4.
 * If the provided string is not a valid ipv4 address, the return ip will be 0.
 *
 * @param value
 * @return esp_ip4_addr_t
 */
esp_ip4_addr_t str2ipv4(const char *const value);

#endif
