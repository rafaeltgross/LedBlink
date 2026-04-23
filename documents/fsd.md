# Functional Specification Document: ESP32-S3 Swiss Clock LED

**Version:** 3.0
**Date:** 2026-04-24
**Project:** SwissLedClock
**Platform:** ESP32-S3-DevKitC-1

---

## 1. Purpose

This firmware turns the onboard WS2812 RGB LED of the ESP32-S3-DevKitC-1 into a visual Swiss clock chime. It connects to WiFi on startup, synchronizes time via NTP (Bern/Zurich timezone), then blinks the LED at every quarter-hour exactly as a traditional Swiss clock bell would ring.

---

## 2. Scope

- WiFi connection (WPA2, 2.4 GHz)
- NTP time synchronization (pool.ntp.org, timezone CET/CEST Bern)
- Quarter-hour LED chime sequence (1–3 blinks)
- Top-of-hour LED chime sequence (4 blinks + N hour strikes)
- 12-hour repeating cycle

Out of scope: UART logging, RTC module, user input, multiple LEDs, BLE.

---

## 3. Hardware Requirements

| Item | Requirement |
|------|-------------|
| MCU | ESP32-S3-DevKitC-1 |
| LED | Onboard WS2812 RGB (GPIO 48) |
| Network | 2.4 GHz WPA2 WiFi access point |
| Power | USB or 3.3 V regulated supply |
| Framework | ESP-IDF 5.4 |

---

## 4. Startup Sequence

```
[POWER ON] → [WiFi connect] → [NTP sync] → [Wait for next :00/:15/:30/:45] → [CHIME loop]
```

The device blocks at WiFi and NTP until both succeed. If WiFi drops, it reconnects automatically. NTP resyncs periodically in the background.

---

## 5. Chime Pattern

| Time | Event | LED Blinks |
|------|-------|------------|
| HH:15 | Quarter 1 | 1 blink |
| HH:30 | Quarter 2 | 2 blinks |
| HH:45 | Quarter 3 | 3 blinks |
| HH:00 | Top of hour | 4 blinks, pause, then N blinks (N = current hour 1–12) |

After the 12:00 chime (4 + 12 blinks) the cycle resets to 1:00.

---

## 6. Functional Requirements

### 6.1 Connectivity

| ID | Requirement |
|----|-------------|
| FR-01 | The firmware shall connect to the WiFi network defined by `WIFI_SSID` and `WIFI_PASS` on startup. |
| FR-02 | If the connection drops, the firmware shall reconnect automatically. |
| FR-03 | The firmware shall synchronize time with `NTP_SERVER` before starting the chime loop. |
| FR-04 | The timezone shall be set to `TIMEZONE` (CET-1CEST, Bern/Zurich). |

### 6.2 Timing

| ID | Requirement |
|----|-------------|
| FR-05 | The firmware shall sleep until the next exact quarter-hour boundary (:00, :15, :30, :45). |
| FR-06 | Each blink shall consist of LED ON for `BLINK_ON_MS` followed by LED OFF for `BLINK_OFF_MS`. |
| FR-07 | At the top of the hour, a pause of `CHIME_PAUSE_MS` shall separate the 4 quarter blinks from the hour strikes. |

### 6.3 Chime Sequence

| ID | Requirement |
|----|-------------|
| FR-08 | At :15 the firmware shall blink 1 time. |
| FR-09 | At :30 the firmware shall blink 2 times. |
| FR-10 | At :45 the firmware shall blink 3 times. |
| FR-11 | At :00 the firmware shall blink 4 times, pause, then blink N times (N = current hour 1–12). |

### 6.4 LED State

| ID | Requirement |
|----|-------------|
| FR-12 | The LED shall be OFF at startup and remain OFF between chime events. |
| FR-13 | The LED shall return to OFF after every blink. |

---

## 7. Configuration Constants

All user-tunable values are defined at the top of `main/led_blink_main.c`:

| Constant | Default | Description |
|----------|---------|-------------|
| `LED_PIN` | `48` | GPIO pin for the WS2812 LED |
| `LED_COLOR_R/G/B` | `20, 20, 20` | RGB color while LED is on (0–255) |
| `BLINK_ON_MS` | `200` | LED on duration per blink (ms) |
| `BLINK_OFF_MS` | `200` | LED off duration between blinks (ms) |
| `CHIME_PAUSE_MS` | `800` | Pause between quarter and hour chimes (ms) |
| `WIFI_SSID` | `"RG-IoT"` | WiFi network name |
| `WIFI_PASS` | `"rafa123$$$"` | WiFi password |
| `NTP_SERVER` | `"pool.ntp.org"` | NTP server |
| `TIMEZONE` | `"CET-1CEST,M3.5.0,M10.5.0/3"` | POSIX timezone string (Bern) |

---

## 8. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | All configurable values shall be defined as named constants at the top of the source file. |
| NFR-02 | Time is NTP-synchronized — no drift accumulates across resets. |
| NFR-03 | The firmware shall compile and run on ESP-IDF 5.4 targeting ESP32-S3. |
| NFR-04 | The firmware shall use the `espressif/led_strip` component for WS2812 control. |

---

## 9. Acceptance Criteria

1. After power-on the device connects to WiFi and syncs time within 30 seconds.
2. At :15 past any hour, exactly 1 blink is observed.
3. At :30, exactly 2 blinks; at :45, exactly 3 blinks.
4. At the top of the hour, exactly 4 blinks followed by a pause and N blinks (N = current hour).
5. After 12:00 (4+12 blinks), the next top-of-hour chime is 4+1 blinks.
6. The LED is off at all other times.
7. If WiFi drops, the device reconnects without requiring a reset.
