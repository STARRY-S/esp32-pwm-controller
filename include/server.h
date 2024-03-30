#ifndef SERVER_H
#define SERVER_H

#include <esp_err.h>
#include <esp_http_server.h>

#include "config.h"

/**
 * @brief start default web server and get the server handle
 *
 * @param port [in] http web server port, default: 80
 * @param handle [out] server handle
 * @return esp_err_t
 */
esp_err_t start_default_http_server(httpd_handle_t *, struct config*);

/**
 * @brief stop controller web server
 *
 * @param handle server handle
 * @return esp_err_t
 */
esp_err_t stop_default_http_server(httpd_handle_t handle);

#endif // SERVER_H
