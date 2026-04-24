# SwissLedClock

A beautiful RGB LED clock on your Waveshare ESP32-S3-Zero that blinks like a Swiss chime clock — with colorful LED shows for every quarter hour and a special "go to sleep" message at 23:30. 🔔

On startup it connects to WiFi and syncs the time automatically via NTP (Bern/Zurich timezone).

## How it works

The LED displays the time with color-coded signals:

| Time | LED Color | Pattern |
|------|-----------|---------|
| Every minute | 🟢 Green | 1 blink (minute indicator) |
| :15 past | 🌈 Rainbow | 1x full color show (Red→Orange→Yellow→Green→Blue→Indigo→Violet) |
| :30 past | 🌈 Rainbow | 2x full color shows |
| :45 past | 🌈 Rainbow | 3x full color shows |
| **Top of hour (00 min)** | 🌈 Rainbow | **4x full color shows**, then 🔴 N red blinks for the hour (1–12) |

After each hour on the hour, an email is sent to `rafael@rgross.ch` with the current time.

### Special case: 23:30 (bedtime)

At 23:30, the clock sends a special email message:
> **RAFA! Time to sleep NOW and STOP DRINKING!**

Then it enters sleep mode for 30 minutes before resuming normal operation.

## Hardware

- **Board:** Waveshare ESP32-S3-Zero
- **LED:** Onboard WS2812 RGB on **GPIO 21**
- **Framework:** ESP-IDF 5.4
- **Networking:** WiFi (RG-IoT) + NTP time sync
- **Email:** HTTP relay via Pi (192.168.0.50:9090)

## Build and flash

```bash
cd ~/SwissLedClock
source ~/esp/esp-idf/export.sh

# Set target (only once)
idf.py set-target esp32s3

# Build and flash
idf.py -p /dev/ttyACM1 build flash
```

> Run `set-target esp32s3` only once, or when switching boards.

## Customise

Open `main/led_blink_main.c` and change the constants at the top:

| Constant | Default | What it does |
|----------|---------|--------------|
| `LED_PIN` | 21 | GPIO do LED WS2812 (21 no Waveshare ESP32-S3-Zero) |
| `BLINK_ON_MS` | 150 ms | Tempo que o LED fica aceso por piscada |
| `BLINK_OFF_MS` | 150 ms | Intervalo entre piscadas |
| `PAUSE_MS` | 600 ms | Pausa entre shows e blinks |
| `WIFI_SSID` | `"RG-IoT"` | Nome da rede WiFi |
| `WIFI_PASS` | `"rafa123$$$"` | Senha da rede WiFi |

## Email notifications

The clock sends email through an HTTP relay on the Pi:

**What gets sent:**
- Every hour on the hour: Email with current time
- 23:30: Special "time to sleep" message with warning about drinking

**How it works:**
1. ESP32 connects to WiFi and syncs time via NTP
2. Sends `GET http://192.168.0.50:9090/send?device=SwissLedClock&subject=<subject>&message=<msg> at <time>`
3. Pi relay (running `workbench_email/pi_mail_relay.py`) receives HTTP and forwards to SMTP

**Run the relay on the Pi:**
```bash
python3 pi_mail_relay.py
# Listening on :9090
```

## Project files

```
main/led_blink_main.c               — firmware principal (relógio suíço com LED RGB)
workbench_email/                    — notificação por email via relay no Pi
  main/workbench_email.c            — firmware ESP32-S3 (WiFi + NTP + HTTP)
  pi_mail_relay.py                  — relay HTTP→SMTP no Pi
documents/fsd.md                    — especificação funcional completa
documents/blink-idea.md             — notas originais do projeto
```

## Features

✅ Automatic time sync via NTP  
✅ Colorful LED shows at quarter hours  
✅ Red hour chimes (1-12 blinks)  
✅ Email notifications every hour  
✅ Special bedtime alarm at 23:30  
✅ WiFi-enabled (works anywhere with connectivity)  
✅ Waveshare ESP32-S3-Zero compatible
