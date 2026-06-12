#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t file_browser_init(const char *base_path);

int file_browser_run(char *selected_path, int max_len);

void file_browser_deinit(void);

#ifdef __cplusplus
}
#endif
