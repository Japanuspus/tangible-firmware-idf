#ifndef __TANGIBLE_QRCAMERA_H__
#define __TANGIBLE_QRCAMERA_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t qrcamera_setup();
int qrcamera_get(char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
#endif