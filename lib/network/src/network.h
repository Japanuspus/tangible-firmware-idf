#ifndef __TANGIBLE_NETWORK_H__
#define __TANGIBLE_NETWORK_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void network_init(void);
esp_err_t post_message(char *msg);

#ifdef __cplusplus
}
#endif
#endif