#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);

void app_check_safe_boot(void);

int app_get_rom_path(char *buf, int buflen);

void app_return_to_launcher(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
