#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "network.h"
#include "qrcamera.h"

#define QR_BUFFER_SIZE 128

static const char *TAG = "blink"; //for log

static char qr_buffer[QR_BUFFER_SIZE];
static uint8_t s_led_state = 0;
static const gpio_num_t BLINK_GPIO = GPIO_NUM_33;

static void blink_led(void)
{
    s_led_state = !s_led_state;
    ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}


// default main task stacksize is 3584
// set in menuconfig -> Component config -> Common ESP-related -> Main task stack size
// raise stack size to 100000 (64000 still fails)
void app_main(void)
{
    ESP_LOGI(TAG, "Configure blink led");
    configure_led();
    ESP_LOGI(TAG, "Configure network");
    network_init();
    ESP_LOGI(TAG, "Configuring camera");
    qrcamera_setup();

    for(int i=0;i<500;i++) {
        ESP_LOGI(TAG, "Loop");
        blink_led();

        ESP_LOGI(TAG, "Running QR count");
        int count = qrcamera_get(qr_buffer, QR_BUFFER_SIZE);
        ESP_LOGI(TAG, "QR count: %d", count);

        esp_err_t err;
        if (count == 1) {
            err = post_message(qr_buffer);
        } else {
            err = post_message("no code");
        }
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Post failed");
        } else {
            ESP_LOGI(TAG, "Post ok");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}