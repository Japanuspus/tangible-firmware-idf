idf_component_register(SRCS network.c
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_client nvs_flash)