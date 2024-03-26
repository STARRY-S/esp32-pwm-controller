#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <sys/stat.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>

#include "storage.h"

#define TAG "STORAGE"

esp_err_t init_storage()
{
	ESP_LOGD(TAG, "init_storage start");
	esp_err_t ret = 0;
	if ((ret = nvs_flash_init()) != ESP_OK) {
		ESP_LOGE(TAG, "nvs_flash_init failed %d", ret);
		return ret;
	}

	esp_vfs_spiffs_conf_t config = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = false
	};
	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	if ((ret = esp_vfs_spiffs_register(&config)) != ESP_OK) {
		ESP_LOGE(TAG, "esp_vfs_spiffs_register failed %d", ret);
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(config.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "esp_spiffs_info failed %d", ret);
		return ret;
	}
	ESP_LOGI(TAG, "spiff partition total: %d, used: %d", total, used);
	ESP_LOGI(TAG, "storage init finished");
	return ESP_OK;
}

int read_file(char** content, const char *filename)
{
	if (filename == NULL || content == NULL) {
		return 0;
	}

	FILE *fd = fopen(filename, "r");
	if (fd == NULL) {
		ESP_LOGE(TAG, "failed to open: %s", filename);
		return 0;
	}

	fseek(fd, 0L, SEEK_END);
	int size = ftell(fd);
	fseek(fd, 0L, SEEK_SET);
	*content = malloc(sizeof(char) * (size+1));
	if (*content == NULL) {
		fclose(fd);
		return 0;
	}
	memset(*content, 0, size+1);
	int num = fread(*content, size, 1, fd);
	if (num == 0) {
		ESP_LOGE(TAG, "failed to read file");
		return 0;
	}
	fclose(fd);
	ESP_LOGD(TAG, "read file: %s, size: %d", filename, size);
	return size;
}

int write_file(char *filename, char *content)
{
	FILE *fd = fopen(filename, "w");
	int ret = fprintf(fd, "%s", content);
	fclose(fd);
	return ret;
}

bool is_regular_file(const char *filename)
{
	struct stat s;
	stat(filename, &s);
	return S_ISREG(s.st_mode);
}
