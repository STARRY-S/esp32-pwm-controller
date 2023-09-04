#include <esp_err.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <string.h>
#include <sys/stat.h>

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
	ESP_LOGI(TAG, "read file: %s, size: %d", filename, size);
	return size;
}

int write_file(char *filename, char *content)
{
	FILE *fd = fopen(filename, "w");
	fprintf(fd, "%s", content);
	fclose(fd);
	return ESP_OK;
}

bool is_regular_file(const char *filename)
{
	struct stat s;
	stat(filename, &s);
	ESP_LOGI(TAG, "file: %s, mode: 0X%X", filename, (unsigned int)s.st_mode);
	return S_ISREG(s.st_mode);
}
