#include <esp_err.h>
#include <esp_log.h>
#include <esp_spiffs.h>

#include "storage.h"

#define TAG "SPIFFS"

void init_spiffs_storage()
{
	ESP_LOGI(TAG, "Start init spiff storage");
	esp_vfs_spiffs_conf_t config = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = false
	};
	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	ESP_ERROR_CHECK(esp_vfs_spiffs_register(&config));

	size_t total = 0, used = 0;
	ESP_ERROR_CHECK(esp_spiffs_info(config.partition_label, &total, &used));
	ESP_LOGI(TAG, "Spiff partition total: %d, used: %d", total, used);
	ESP_LOGI(TAG, "Storage init finished");
}
