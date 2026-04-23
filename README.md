# SwissLedClock

The onboard RGB LED on your ESP32-S3 blinks like a Swiss clock — once every 15 minutes to mark the quarter hours, and at the top of every hour it strikes four times plus once for each hour (just like the bells on a real clock).

On startup it connects to WiFi and syncs the time automatically via NTP (Bern/Zurich timezone).

## How it works

| Time | What you see |
|------|--------------|
| :15 past | 1 blink |
| :30 past | 2 blinks |
| :45 past | 3 blinks |
| Top of hour | 4 blinks, short pause, then N blinks for the hour (1–12) |

After 12 o'clock the cycle starts over at 1.

## Hardware

- **Board:** ESP32-S3-DevKitC-1
- **LED:** Onboard WS2812 RGB on GPIO 48
- **Framework:** ESP-IDF 5.4

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -C ~/esp/SwissLedClock set-target esp32s3
idf.py -C ~/esp/SwissLedClock -p /dev/ttyACM1 build flash monitor
```

> Run `set-target esp32s3` only once, or when switching boards.

## Customise

Open `main/led_blink_main.c` and change the constants at the top:

| Constant | Default | What it does |
|----------|---------|--------------|
| `LED_PIN` | 48 | GPIO do LED WS2812 (48 no S3-DevKitC-1) |
| `BLINK_ON_MS` | 200 ms | Tempo que o LED fica aceso por piscada |
| `BLINK_OFF_MS` | 200 ms | Intervalo entre piscadas |
| `CHIME_PAUSE_MS` | 800 ms | Pausa entre batidas dos quartos e da hora |
| `WIFI_SSID` | `"RG-IoT"` | Nome da rede WiFi |
| `WIFI_PASS` | `"rafa123$$$"` | Senha da rede WiFi |
| `LED_COLOR_R/G/B` | 20, 20, 20 | Cor do LED (0–255 cada canal) |

## Project files

```
main/led_blink_main.c   — código do firmware
documents/fsd.md        — especificação funcional completa
documents/blink-idea.md — notas originais do projeto
```
