/* Configuration constants — change only here (NFR-03) */
#define LED_PIN      48      /* GPIO48 = onboard WS2812 RGB LED on ESP32-S3-DevKitC-1 */
#define LED_ON_MS    500
#define LED_OFF_MS   500
#define BLINK_COUNT  3
#define LED_COLOR_R  16      /* RGB color when LED is ON (0-255 each channel) */
#define LED_COLOR_G  16
#define LED_COLOR_B  16

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

void app_main(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = LED_PIN,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    led_strip_handle_t strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));

    /* FR-07: LED OFF at start */
    led_strip_clear(strip);

    for (int i = 0; i < BLINK_COUNT; i++) {
        led_strip_set_pixel(strip, 0, LED_COLOR_R, LED_COLOR_G, LED_COLOR_B);
        led_strip_refresh(strip);
        vTaskDelay(LED_ON_MS / portTICK_PERIOD_MS);

        led_strip_clear(strip);
        vTaskDelay(LED_OFF_MS / portTICK_PERIOD_MS);
    }

    /* FR-04 / FR-07: idle forever, LED stays OFF */
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
