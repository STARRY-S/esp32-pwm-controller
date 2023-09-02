#ifndef SERVER_H
#define SERVER_H

#include <esp_err.h>
#include <esp_http_server.h>

httpd_handle_t start_webserver(uint16_t port);
esp_err_t stop_webserver(httpd_handle_t server);

#endif // SERVER_H
