#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_http_client.h"
#include "nvs_flash.h"

#define TAG         "WORKBENCH"
#define WIFI_SSID   "RG-IoT"
#define WIFI_PASS   "rafa123$$$"
#define RELAY_URL   "http://192.168.0.50:9090/send"

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi desconectado, reconectando...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_cfg = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando ao WiFi %s...", WIFI_SSID);
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();

    /* Sincronizar horario via NTP */
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    ESP_LOGI(TAG, "Aguardando sincronizacao NTP...");
    time_t now = 0;
    struct tm ti = {0};
    for (int i = 0; i < 30 && ti.tm_year < (2020 - 1900); i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &ti);
    }

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    time(&now);
    localtime_r(&now, &ti);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S %Z", &ti);
    ESP_LOGI(TAG, "Horario: %s", time_str);

    /* Montar URL com o horario e enviar para o relay no Pi */
    char url[256];
    snprintf(url, sizeof(url), "%s?device=ESP32-S3&time=%s", RELAY_URL, time_str);

    /* Substituir espaços por %20 */
    for (char *p = url + strlen(RELAY_URL); *p; p++) {
        if (*p == ' ') *p = '+';
    }

    ESP_LOGI(TAG, "Enviando para relay: %s", url);

    esp_http_client_config_t http_cfg = {
        .url            = url,
        .timeout_ms     = 10000,
        .method         = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP status=%d", status);
        if (status == 200) {
            ESP_LOGI(TAG, "Email enviado com sucesso!");
        } else {
            ESP_LOGE(TAG, "Relay retornou erro %d", status);
        }
    } else {
        ESP_LOGE(TAG, "HTTP falhou: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
