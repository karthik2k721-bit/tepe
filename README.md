# Follow-Me Trolley (ESP32 + Arduino + Ultrasonic + Buzzer)

This project implements a follow-me trolley alert system using two controllers:

- Arduino handles ultrasonic sensing and buzzer output.
- ESP32 handles WiFi-based target tracking and sends buzzer mode commands to Arduino.

The system behavior is:

- If obstacle distance is less than 5 cm, obstacle override is active and buzzer sounds continuously.
- If obstacle distance is 5 cm or more, system returns to normal tracking mode.
- In tracking mode:
	- Near target -> buzzer pattern: beep beep beep every 2 seconds.
	- Far target -> buzzer pattern: beep beep beep every 5 seconds.

## Files

- ardiuno.ino
	- Reads ultrasonic distance.
	- Sends distance to ESP32 through UART.
	- Receives buzzer mode commands from ESP32 and drives buzzer.
- esp32.ino
	- Scans nearby WiFi networks.
	- Lets user select target SSID through serial terminal.
	- Smooths RSSI values and decides near/far mode.
	- Applies ultrasonic override from Arduino distance.

## Hardware Connections

### Arduino

- Ultrasonic sensor
	- TRIG -> D9
	- ECHO -> D10
	- VCC -> 5V
	- GND -> GND
- Buzzer
	- Buzzer signal -> D8
	- GND -> GND

### UART Link Between ESP32 and Arduino

- ESP32 TX2 (GPIO17) -> Arduino RX (software or hardware serial input used by sketch serial wiring)
- ESP32 RX2 (GPIO16) -> Arduino TX
- Common ground between boards is mandatory.

Note:
- Arduino sketch uses Serial at 9600 baud for inter-board communication.
- ESP32 sketch uses Serial2 at 9600 baud on pins 16 and 17.

## Software Architecture

## 1) Arduino Logic

Arduino uses a buzzer state machine:

- BUZZER_OFF
- BUZZER_TRACK_NEAR
- BUZZER_TRACK_FAR
- BUZZER_ALERT

### Distance sensing

- `getDistance()` sends a 10 us trigger pulse and measures echo pulse width.
- `pulseIn(..., 30000)` timeout avoids lockups when no echo is returned.
- Distance is sent to ESP32 every `DISTANCE_SEND_MS` (100 ms).

### Command handling from ESP32

Arduino reads 1-byte commands from serial:

- `A` -> Continuous buzzer ON (obstacle override)
- `N` -> Near tracking burst mode
- `F` -> Far tracking burst mode
- `S` or `C` -> Buzzer OFF

### Buzzer patterns

- Alert mode (`A`): always HIGH.
- Tracking modes (`N`/`F`): burst generator creates 3 short beeps (`beep beep beep`).
- Interval between bursts:
	- Near: 2000 ms
	- Far: 5000 ms

## 2) ESP32 Logic

ESP32 performs WiFi tracking and mode selection.

### Tracking selection

- In idle mode, ESP32 scans WiFi and prints indexed list to serial monitor.
- User enters network index.
- Selected SSID becomes tracking target.

### Smooth continuous tracking features

- Non-blocking WiFi scan cycle:
	- `requestScan()` starts async scan.
	- `processScanResult()` reads result without freezing loop.
- RSSI smoothing:
	- Exponential moving average filter.
	- `filteredRssi = alpha * raw + (1 - alpha) * previous`
	- Current `alpha = 0.35`.
- Hysteresis thresholds reduce mode flapping:
	- Enter near when `filteredRssi >= -62`.
	- Return far when `filteredRssi <= -68`.

### Ultrasonic override

- ESP32 receives distance lines from Arduino over Serial2.
- If distance `< 5`, ESP32 sends `A` and returns immediately.
- Once distance `>= 5`, ESP32 resumes near/far command logic.

### UART traffic optimization

- `sendCommandIfChanged()` only sends command when mode changes.
- Reduces unnecessary serial writes and jitter.

## Command Protocol

Single-byte commands from ESP32 to Arduino:

- `A`: obstacle override, continuous buzzer
- `N`: near target tracking pattern
- `F`: far target tracking pattern
- `S`: stop buzzer
- `C`: stop buzzer (alternate stop code)

Distance from Arduino to ESP32:

- ASCII float as line, for example: `12.37\n`

## How To Run

1. Flash `ardiuno.ino` to Arduino board.
2. Flash `esp32.ino` to ESP32 board.
3. Wire UART and ground between both boards.
4. Open ESP32 serial monitor at 115200 baud.
5. Wait for WiFi scan list.
6. Enter index of phone hotspot SSID to track.
7. Move target device and verify near/far buzzer pattern.
8. Place obstacle within 5 cm of ultrasonic sensor and verify continuous override.

## Tuning Parameters

You can tune behavior from constants.

### Arduino

- `NEAR_BURST_INTERVAL_MS` (default 2000)
- `FAR_BURST_INTERVAL_MS` (default 5000)
- `BURST_TOGGLE_MS` (default 120)
- `DISTANCE_SEND_MS` (default 100)

### ESP32

- `SCAN_INTERVAL_MS` (default 700)
- `RSSI_ALPHA` (default 0.35)
	- Higher value = faster but noisier response.
	- Lower value = smoother but slower response.
- `RSSI_NEAR_ENTER` and `RSSI_FAR_ENTER`
	- Wider gap between thresholds = more stable switching.

## Common Issues And Fixes

1. Tracking updates are slow
- Reduce `SCAN_INTERVAL_MS` moderately (for example 500 to 700).
- Keep async scan design; avoid adding long delay calls.

2. Near/far keeps toggling rapidly
- Increase hysteresis gap:
	- Make near threshold higher (for example -60)
	- Make far threshold lower (for example -70)
- Reduce `RSSI_ALPHA` for more smoothing.

3. No distance update on ESP32
- Verify UART TX/RX crossing between boards.
- Verify both use 9600 on inter-board serial link.
- Confirm common ground.

4. Buzzer not sounding
- Verify buzzer wiring and pin D8.
- Confirm Arduino receives command bytes over serial.

## Future Improvements

- Add Bluetooth RSSI fallback when target SSID is unavailable.
- Add lost-target timeout and safe behavior strategy.
- Add motor control integration for full autonomous trolley following.
- Add calibration mode to auto-estimate RSSI thresholds per environment.
