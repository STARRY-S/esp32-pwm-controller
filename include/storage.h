#ifndef STORAGE_H
#define STORAGE_H

#include <esp_err.h>

/**
 * @brief init_storage initializes esp32 storage.
 */
esp_err_t init_storage();

/**
 * @brief read_file reads file data.
 * WARN: this function is only used for small sized file.
 *
 * @param p pointer points to the string
 * @param filename
 * @return int read data length, 0 if failed
 */
int read_file(char** p, const char *filename);

/**
 * @brief write_file writes content to the file.
 *
 * @param filename
 * @param content
 * @return int write data length, 0 if failed
 */
int write_file(char *filename, char *content);

/**
 * @brief is_regular_file detects if the file is a regular file
 * by using S_ISREG.
 *
 * @param filename
 * @return bool
 */
bool is_regular_file(const char *filename);

#endif
