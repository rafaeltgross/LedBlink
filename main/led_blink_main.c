#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "led_strip.h"
#include "esp_log.h"
#include <time.h>
#include <string.h>

#define TAG               "SwissLedClock"
#define LED_PIN           21       /* GPIO21 = LED WS2812 on Waveshare ESP32-S3-Zero */
#define BLINK_ON_MS       200
#define BLINK_OFF_MS      200
#define PAUSE_MS          800

#define WIFI_SSID         "RG-IoT"
#define WIFI_PASS         "rafa123$$$"
#define NTP_SERVER        "pool.ntp.org"
#define TIMEZONE          "CET-1CEST,M3.5.0,M10.5.0/3"
#define RELAY_URL         "http://192.168.0.50:9090/send"

static EventGroupHandle_t s_wifi_events;
#define WIFI_CONNECTED_BIT  BIT0
static int last_hour = -1;
static int last_minute_30 = -1;

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

static void blink_color(led_strip_handle_t strip, int count, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < count; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(strip));
        vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
        ESP_ERROR_CHECK(led_strip_clear(strip));
        if (i < count - 1)
            vTaskDelay(pdMS_TO_TICKS(BLINK_OFF_MS));
    }
}

static void send_email(const char *subject, const char *message)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", &t);

    char url[512];
    snprintf(url, sizeof(url), "%s?device=SwissLedClock&subject=%s&message=%s at %s",
             RELAY_URL, subject, message, time_str);

    for (char *p = url + strlen(RELAY_URL); *p; p++) {
        if (*p == ' ') *p = '+';
    }

    ESP_LOGI(TAG, "Sending email: %s", url);

    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 10000,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Email sent, HTTP status=%d", status);
    } else {
        ESP_LOGE(TAG, "Email failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void wait_for_next_minute(void)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    int secs_past = t.tm_sec;
    int secs_to_next = 60 - secs_past;

    if (secs_to_next > 2)
        vTaskDelay(pdMS_TO_TICKS((secs_to_next - 1) * 1000));

    do {
        vTaskDelay(pdMS_TO_TICKS(100));
        time(&now);
        localtime_r(&now, &t);
    } while (t.tm_sec != 0);
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
        .led_model = LED_MODEL_WS2812,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    led_strip_handle_t strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));
    ESP_ERROR_CHECK(led_strip_clear(strip));

    ESP_LOGI(TAG, "SwissLedClock started!");

    while (1) {
        wait_for_next_minute();

        time_t now;
        struct tm t;
        time(&now);
        localtime_r(&now, &t);

        int hour = t.tm_hour;
        int minute = t.tm_min;

        /* Check if it's time to go to sleep (23:30) */
        if (hour == 23 && minute == 30 && last_minute_30 != 30) {
            last_minute_30 = 30;
            ESP_LOGI(TAG, "Good night! Going to sleep...");
            blink_color(strip, 3, 255, 165, 0);  /* 3 orange blinks */
            send_email("Sleep", "Time+to+sleep");
            vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));  /* Sleep for 30 minutes */
        } else if (hour != 23 || minute != 30) {
            last_minute_30 = -1;
        }

        /* Top of hour: yellow blink, then red blinks for hour, then send email */
        if (minute == 0) {
            if (last_hour != hour) {
                last_hour = hour;
                ESP_LOGI(TAG, "Top of hour! %02d:%02d", hour, minute);

                /* Yellow blink for top of hour */
                blink_color(strip, 1, 255, 255, 0);
                vTaskDelay(pdMS_TO_TICKS(PAUSE_MS));

                /* Red blinks for current hour */
                int blinks = hour % 12;
                if (blinks == 0) blinks = 12;
                blink_color(strip, blinks, 255, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(PAUSE_MS));

                /* Send email */
                char time_str[32];
                strftime(time_str, sizeof(time_str), "%H:%M", &t);
                send_email("Hour", time_str);
            }
        } else {
            /* Green blink for every minute (not on the hour) */
            blink_color(strip, 1, 0, 255, 0);
        }
    }
}
