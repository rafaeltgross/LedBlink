idf_component_register(SRCS "led_blink_main.c"
                        INCLUDE_DIRS "."
                        REQUIRES led_strip nvs_flash esp_wifi esp_netif esp_event esp_http_client)
