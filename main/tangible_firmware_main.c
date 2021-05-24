#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "network.h"
#include "qrcamera.h"

#define QR_BUFFER_SIZE 128

static const gpio_num_t photo_drive_pin = GPIO_NUM_12;
static const gpio_num_t photo_read_pin = GPIO_NUM_13;
//static const gpio_num_t BLINK_GPIO = GPIO_NUM_33;

static const char *TAG = "tgbl"; //for log

static char qr_buffer[QR_BUFFER_SIZE];
//static uint8_t s_led_state = 0;

static void card_sensor_setup(void)
{
    gpio_reset_pin(photo_read_pin);
    gpio_set_direction(photo_read_pin, GPIO_MODE_INPUT);
    gpio_reset_pin(photo_drive_pin);
    gpio_set_direction(photo_drive_pin, GPIO_MODE_OUTPUT);
    gpio_set_drive_capability(photo_drive_pin, GPIO_DRIVE_CAP_0); //~10mA
}

bool card_sensor_read(void) {
    gpio_set_level(photo_drive_pin, 1);
    vTaskDelay(1);
    int read_level = gpio_get_level(photo_read_pin);
    gpio_set_level(photo_drive_pin, 0);
    return read_level == 1;
}

bool attempt_qr_read(void) {
    int count = qrcamera_get(qr_buffer, QR_BUFFER_SIZE);
    if (count !=1) {
        return false;
    }
    esp_err_t err = post_message(qr_buffer);
    return err==ESP_OK;
}

// default main task stacksize is 3584
// set in menuconfig -> Component config -> Common ESP-related -> Main task stack size
// raise stack size to 100000 (64000 still fails)
void app_main(void)
{
    ESP_LOGI(TAG, "Configure network");
    network_init();
    ESP_LOGI(TAG, "Configuring camera");
    qrcamera_setup();
    ESP_LOGI(TAG, "Configuring photo sensor");
    card_sensor_setup();

    int state = 0;
    // 0: no card
    // 1: card, qr

    while(true) {
        int has_card = card_sensor_read();
        ESP_LOGI(TAG, "Loop start. State: %d, Has card: %d", state, has_card);

        if (!has_card) {
            if (state==1) {
                post_message("no card");
                state=0;
            }
        } else {
            //has card
            if (state==0) {
                if (attempt_qr_read()) {
                    state=1;
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}