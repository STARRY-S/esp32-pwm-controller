#ifndef STORAGE_H
#define STORAGE_H

void init_spiffs_storage();
int read_file(char** content, const char *filename);
bool is_regular_file(const char *filename);

#endif