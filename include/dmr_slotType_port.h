#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int dmr_get_color_code(const uint8_t* header33, size_t len, uint8_t* cc);
int dmr_get_data_type(const uint8_t* header33, size_t len, uint8_t* dt);

#ifdef __cplusplus
}
#endif
