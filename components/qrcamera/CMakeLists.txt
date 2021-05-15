idf_component_register(SRCS decode.c identify.c quirc.c version_db.c qrcamera.c
                       INCLUDE_DIRS "include"
                       REQUIRES esp32-camera)