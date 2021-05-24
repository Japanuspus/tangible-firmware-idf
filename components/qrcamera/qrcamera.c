#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // for memset

#include "esp_log.h"
#include "esp_system.h"
#include "esp_camera.h"

#include "quirc.h"
#include "quirc_internal.h"
#include "qrcamera.h"
#include "driver/gpio.h"

static camera_config_t camera_config;
static struct quirc qr_recognizer;
static struct quirc_data qr_data;
static struct quirc_code qr_code;

static const char *TAG = "qrcamera"; //for log

static const gpio_num_t flash_pin = GPIO_NUM_4;

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

void dump_ram_state() {
    //ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());
}


static void dump_data(const struct quirc_data *data)
{
    ESP_LOGI(TAG, "Version: %d", data->version);
    ESP_LOGI(TAG, "ECC level: %c", "MLHQ"[data->ecc_level]);
    ESP_LOGI(TAG, "Mask: %d", data->mask);
    ESP_LOGI(TAG, "Data type: %d (%s)", data->data_type, data_type_str(data->data_type));
    ESP_LOGI(TAG, "Length: %d", data->payload_len);
    ESP_LOGI(TAG, "Payload: %s", data->payload);
    if (data->eci) {
        ESP_LOGI(TAG, "ECI: %d", data->eci);
    }
}


int process_frame_buffer(camera_fb_t *fb, char *out, size_t out_size) {
    quirc_analyze_buffer(&qr_recognizer, fb->buf, fb->width, fb->height);
 
    // Check number of qr codes    
    int count = quirc_count(&qr_recognizer);
    ESP_LOGI(TAG, "Found %d qr codes", count);
    if (count != 1) {
        return 0;
    }
    // Exactly one code: decode
    quirc_extract(&qr_recognizer, 0, &qr_code); //0: index
 
    ESP_LOGI(TAG, "Extract complete, decoding");
    //Decode a QR-code, returning the payload data.
    quirc_decode_error_t err = quirc_decode(&qr_code, &qr_data);
    if (err) {
        ESP_LOGI(TAG, "Decoding FAILED: %s\n", quirc_strerror(err));
        return -10;
    }
    dump_data(&qr_data);
    if (qr_data.payload_len>=out_size) {
        ESP_LOGI(TAG, "Oversize payload: %d > %d", out_size, qr_data.payload_len);
        return -20;
    }
    ESP_LOGI(TAG, "Successfully decoded unique QR");
    memcpy(out, qr_data.payload, qr_data.payload_len);
    out[qr_data.payload_len]=0;
    return 1;
}

void set_flash_state(bool onoff) {
    gpio_set_level(flash_pin, onoff?1:0);
}

void setup_flash(void) {
    gpio_reset_pin(flash_pin);
    set_flash_state(false);
    gpio_set_direction(flash_pin, GPIO_MODE_OUTPUT);
    gpio_set_drive_capability(flash_pin, GPIO_DRIVE_CAP_0); //~10mA
    set_flash_state(false);
}

esp_err_t qrcamera_setup() {
    memset(&qr_recognizer, 0, sizeof(qr_recognizer));
    setup_flash();
    return setup_camera();
}

// qr_recognizer must be initialized
// <0: failure
// -2: no framebuffer
// 0: ok, but no unique match
// 1: exactly one match
int qrcamera_get(char *out, size_t out_size) {
    camera_fb_t *fb = NULL;
    dump_ram_state();
    set_flash_state(true);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    fb = esp_camera_fb_get();
    set_flash_state(false);
    if (!fb) {
        ESP_LOGI(TAG, "Camera capture failed");
        return -2;
    }
    ESP_LOGI(TAG, "Camera capture success");
    dump_ram_state();
    int res = process_frame_buffer(fb, out, out_size);

    esp_camera_fb_return(fb);
    ESP_LOGI(TAG, "Frame buffer returned");
    dump_ram_state();

    return res;
}