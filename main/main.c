#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "network.h"

static const char *TAG = "blink"; //for log

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
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    configure_led();
    network_init();

    for(int i=0;i<5;i++) {
        blink_led();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_err_t err = post_message("The foo");
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Post failed");
            break;
        }
    }
}