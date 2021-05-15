#include "quirc.h";
#include "esp_camera.h"
static struct quirc qr_recognizer;
camera_config_t camera_config;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// packages/esp32/hardware/esp32/1.0.5/tools/sdk/include/esp32-camera/esp_camera.h
// https://github.com/espressif/esp32-camera
// https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h

esp_err_t setup_camera() {
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pin_d0 = Y2_GPIO_NUM;
    camera_config.pin_d1 = Y3_GPIO_NUM;
    camera_config.pin_d2 = Y4_GPIO_NUM;
    camera_config.pin_d3 = Y5_GPIO_NUM;
    camera_config.pin_d4 = Y6_GPIO_NUM;
    camera_config.pin_d5 = Y7_GPIO_NUM;
    camera_config.pin_d6 = Y8_GPIO_NUM;
    camera_config.pin_d7 = Y9_GPIO_NUM;
    camera_config.pin_xclk = XCLK_GPIO_NUM;
    camera_config.pin_pclk = PCLK_GPIO_NUM;
    camera_config.pin_vsync = VSYNC_GPIO_NUM;
    camera_config.pin_href = HREF_GPIO_NUM;
    camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
    camera_config.pin_sscb_scl = SIOC_GPIO_NUM;
    camera_config.pin_pwdn = PWDN_GPIO_NUM;
    camera_config.pin_reset = RESET_GPIO_NUM;
    camera_config.xclk_freq_hz = 10000000;
    camera_config.pixel_format = PIXFORMAT_GRAYSCALE; 
    camera_config.frame_size = FRAMESIZE_VGA;  // set picture size, FRAMESIZE_VGA (640x480), see sensor.h
    // camera_config.jpeg_quality = 15;           // quality of JPEG output. 0-63 lower means higher quality
    camera_config.fb_count = 1;              // 1: Wait for V-Synch // 2: Continous Capture (Video)

    return esp_camera_init(&camera_config);
}

esp_err_t setup_qrcamera() {
    memset(&qr_recognizer, 0, sizeof(recognizer));
    return setup_camera();
}

static const char *data_type_str(int dt)
{
    switch (dt) {
    case QUIRC_DATA_TYPE_NUMERIC:
        return "NUMERIC";
    case QUIRC_DATA_TYPE_ALPHA:
        return "ALPHA";
    case QUIRC_DATA_TYPE_BYTE:
        return "BYTE";
    case QUIRC_DATA_TYPE_KANJI:
        return "KANJI";
    }
    return "unknown";
}

void dump_ram_state() {}

// qr_recognizer must be initialized
// <0: failure
// 0: ok, but no or more than 1 match
// 1: exactly one match
int qr_recognize() 
{   
    camera_fb_t *fb = NULL;

    if (camera_config.frame_size > FRAMESIZE_SVGA) {
        ESP_LOGD(TAG, "Camera Frame Size err %d, support maxsize is QVGA", (camera_config.frame_size));
        return -1;
    }

    dump_ram_state();
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGD(TAG, "Camera capture failed");
        return -2;
    }
    ESP_LOGD(TAG, "Camera capture success");
    dump_ram_state();

    qr_recognizer.w = fb->w;
    qr_recognizer.h = fb->h;
    qr_recognizer.image = fb.buf;

    quirc_end(qr_recognizer);


    esp_camera_fb_return(fb);
    ESP_LOGD(TAG, "Frame buffer returned");
    dump_ram_state();
    
    // Return the number of QR-codes identified in the last processed image.
    return quirc_count(qr_recognizer);
}

static void dump_data(const struct quirc_data *data)
{
    ESP_LOGD(TAG, "Version: %d\n", data->version);
    ESP_LOGD(TAG, "ECC level: %c\n", "MLHQ"[data->ecc_level]);
    ESP_LOGD(TAG, "Mask: %d\n", data->mask);
    ESP_LOGD(TAG, "Data type: %d (%s)\n", data->data_type, data_type_str(data->data_type));
    ESP_LOGD(TAG, "Length: %d\n", data->payload_len);
    ESP_LOGD(TAG, "Payload: %s\n", data->payload);
    if (data->eci) {
        ESP_LOGD(TAG, "ECI: %d\n", data->eci);
    }
}


int qr_unique(camera_config_t *camera_config, struct quirc *qr_recognizer, struct quirc_data *data) {
    int res=qr_recognize(camera_config, qr_recognizer);
    if (res<0) {return res;}
    ESP_LOGD(TAG, "Found %d QR code matches", res);
    if (res==0 || res>1) {return res;}

    struct quirc_code code;

    // Extract the QR-code specified by the given index.
    quirc_extract(qr_recognizer, 0, &code);

    //Decode a QR-code, returning the payload data.
    quirc_decode_error_t err = quirc_decode(&code, data);
    if (err) {
        ESP_LOGD(TAG, "Decoding FAILED: %s\n", quirc_strerror(err));
        return -10;
    }
    ESP_LOGD(TAG, "Successfully decoded unique QR");
    dump_data(data);
    return 1;
}