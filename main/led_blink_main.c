#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "led_strip.h"
#include <time.h>

/* Configuração — alterar apenas aqui */
#define LED_PIN           48      /* GPIO48 = LED WS2812 onboard no ESP32-S3-DevKitC-1 */
#define LED_COLOR_R       20
#define LED_COLOR_G       20
#define LED_COLOR_B       20
#define BLINK_ON_MS       200
#define BLINK_OFF_MS      200
#define CHIME_PAUSE_MS    800
#define QUARTER_CHIMES    4

#define WIFI_SSID         "RG-IoT"
#define WIFI_PASS         "rafa123$$$"
#define NTP_SERVER        "pool.ntp.org"
#define TIMEZONE          "CET-1CEST,M3.5.0,M10.5.0/3"  /* Berna/Zurique */

static EventGroupHandle_t s_wifi_events;
#define WIFI_CONNECTED_BIT  BIT0

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
        esp_wifi_connect();
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
}

static void wifi_connect(void)
{
    s_wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi_event, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    xEventGroupWaitBits(s_wifi_events, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
}

static void sync_time(void)
{
    setenv("TZ", TIMEZONE, 1);
    tzset();

    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
    esp_netif_sntp_init(&cfg);
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000)) != ESP_OK) {}
}

static void blink(led_strip_handle_t strip, int count)
{
    for (int i = 0; i < count; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0, LED_COLOR_R, LED_COLOR_G, LED_COLOR_B));
        ESP_ERROR_CHECK(led_strip_refresh(strip));
        vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
        ESP_ERROR_CHECK(led_strip_clear(strip));
        if (i < count - 1)
            vTaskDelay(pdMS_TO_TICKS(BLINK_OFF_MS));
    }
}

static void wait_for_next_quarter(void)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    int secs_past = (t.tm_min % 15) * 60 + t.tm_sec;
    int secs_to_next = 15 * 60 - secs_past;

    if (secs_to_next > 2)
        vTaskDelay(pdMS_TO_TICKS((secs_to_next - 1) * 1000));

    do {
        vTaskDelay(pdMS_TO_TICKS(100));
        time(&now);
        localtime_r(&now, &t);
    } while (!(t.tm_sec == 0 && t.tm_min % 15 == 0));
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_connect();
    sync_time();

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
    ESP_ERROR_CHECK(led_strip_clear(strip));

    while (1) {
        wait_for_next_quarter();

        time_t now;
        struct tm t;
        time(&now);
        localtime_r(&now, &t);

        int quarter = t.tm_min / 15;
        if (quarter == 0) {
            int hour = t.tm_hour % 12;
            if (hour == 0) hour = 12;
            blink(strip, QUARTER_CHIMES);
            vTaskDelay(pdMS_TO_TICKS(CHIME_PAUSE_MS));
            blink(strip, hour);
        } else {
            blink(strip, quarter);
        }
    }
}
