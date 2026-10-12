# ESP32-S3 Lab

**Get the most out of an ESP32-S3 with nothing but the bare board and a phone.**

ESP32-S3 Lab is a single Arduino sketch that turns an ESP32-S3-DevKitC-1 into a small
hands-on lab. The board starts its own WiFi network. When you connect, a captive portal
opens a web app on your phone. The app shows what the chip can do: two cores, vector
instructions, hardware crypto, capacitive touch, Bluetooth LE, 16 MB of flash and
8 MB of PSRAM.

You need no extra parts, no internet connection and no app to install. A USB cable and a
phone (or laptop) are enough.

The code is written for beginners. Most lines have a short comment that explains *what*
happens and *why*.

---

## Contents

- [Features](#features)
- [Hardware](#hardware)
- [Software setup](#software-setup)
- [Usage](#usage)
- [Benchmark results](#benchmark-results)
- [Architecture](#architecture)
- [How it works](#how-it-works)
- [HTTP API and WebSocket reference](#http-api-and-websocket-reference)
- [Serial console](#serial-console)
- [Files](#files)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Features

The web app has nine tabs.

### Dashboard
- Chip temperature, uptime, free RAM and free PSRAM
- Web requests per second, and how many devices are connected to the WiFi network (with the signal strength of each one in dBm)
- Load per core (core 0 and core 1), as numbers and as a live graph
- Graphs of the temperature (last 2 minutes) and of the web requests per second
- State of the BOOT button, and the type of connection: live WebSocket (with the round-trip time) or plain polling

### LED
- Color picker and brightness slider for the onboard RGB LED
- 8 effects: off, solid, breathe, rainbow, disco, fire, police and temperature (the color goes from blue to red as the chip warms up from 30 to 60 °C)
- The effects run in their own task on core 0 at 50 Hz, so the web server on core 1 never has to wait for them

### Music
A small groove box. The browser makes every sound itself with the Web Audio API. No audio
samples are used.
- Two sets: **deep house** and **afro house**, each with its own drum kit and sound
- 10 tracks per set (8 drum tracks + bass + chords) in a 16-step grid. Tap a cell to cycle off / full / soft. Hi-hat cells can also be rolls (2, 3 or 4 hits in one step). A soft bass note plays one octave higher.
- 4 patterns (A-D) per set. While the beat plays, a new pattern starts on the next bar. Copy, paste and clear work on the pattern you are editing.
- 4 chord progressions per set with jazzy m9/maj9 chords (the bass plays the root), and a key selector from E minor to Eb minor
- Tempo (90-135 BPM), swing and volume
- **Random mode**: at the start of a bar it can jump to another pattern, and every bar gets small variations. A *wildness* slider sets how much changes. Extra notes are shown in yellow, removed notes are faded.
- **Live pads** switch groups on or off right away: Kick, Clap, Hats, Percussion, Bass, Chords and Synth. There are also *New pattern* and *Fill now* buttons, and an automatic fill every 4 bars.
- **DJ effects**: a filter (left = dull, right = thin), a 2-bar **build-up** with a riser and a drop, and **stutter** (hold the button)
- **Synth on the touch pins**: GPIO1 is the kick and GPIO2 is the clap. The 12 pins GPIO3-GPIO14 play notes in the current key. You can choose from 3 sounds and 4 scales, and set glide, reverb, echo (in time with the beat) and brightness.
- A drawing of the board with the real pin layout. You can also hold the keys on the screen, and each key glows in its own color.
- Drum taps on GPIO1/GPIO2 are recorded on the nearest step. When the beat is stopped, the first drum tap starts it on beat 1, at the moment you touched the pin.
- The **BOOT button** starts and stops the beat
- The **onboard LED flashes along with the kick and clap** (see [beat-synced LED](#beat-synced-led))
- **Save and load beats** on the ESP32 itself: 4 slots per set. A slot holds all 4 patterns, the tempo, swing, chord progression and key. Because the beats live on the ESP32, you can load them again on another phone.

### Reaction game
- Tap *Start*. The LED turns red, and after a random 1.5-4.5 s it turns green. Press BOOT as fast as you can.
- The BOOT button uses a hardware **interrupt** with `micros()` timestamps, so the time is exact to the microsecond.
- A press within 100 ms counts as a guess ("too early"). No press within 3 s means "too late".
- The record is stored in flash (NVS, through `Preferences`), so it survives a restart.

### Fractal
- The ESP32 computes a 400 x 300 Mandelbrot image (200 x 150 without PSRAM). The phone only adds the colors.
- Tap to zoom in 2x. There are also *Zoom out*, *Reset* and a **1 vs 2 cores** comparison.
- Shows the computation time, the iteration limit (it rises with the zoom, up to 255), the zoom factor and the number of iteration steps per second
- Uses both cores with an atomic row counter (see [How it works](#fractal-on-two-cores))

### Benchmark
- 13 short speed tests of about 0.3 s each, in five groups: compute, cores, SIMD, memory and hardware
- Each test shows a bar compared with the best result in its group
- At the end the page sums up the main ratios: float vs double, 2 cores vs 1, SIMD vs plain C, and internal RAM vs PSRAM
- See [Benchmark results](#benchmark-results) for numbers measured on a real board

### WiFi scan
- Lists nearby 2.4 GHz networks, strongest first, with channel, signal strength (dBm and 4 bars) and whether the network is open

### Bluetooth
- A 5-second Bluetooth LE scan. It shows up to 60 devices, nearest first, with name, manufacturer (from the company ID in the advertisement), signal strength and a rough distance.
- The Bluetooth stack (NimBLE) only starts at the first scan, so it uses no RAM until you need it
- WiFi and Bluetooth share one radio, so a WiFi scan and a BLE scan never run at the same time

### Info
- Chip model and revision, cores and clock speed, flash, RAM, PSRAM, WiFi network and channel, MAC address, ESP-IDF version, fractal resolution, free beat storage and the reason for the last reset
- "Who does what": which work runs on which core
- A **live FreeRTOS task list** (name, core, priority, CPU %, free stack) that refreshes every 2 seconds
- A restart button (tap twice to confirm)

### Under the hood
- **Live WebSocket link** on port 81. The ESP32 pushes its status 25 times per second, and right away when a pin is touched or BOOT is pressed. If the WebSocket fails, the page falls back to polling `/status`.
- **Beat-synced LED**: the page tells the ESP32 in advance when a kick or clap will sound. A FreeRTOS queue passes these flashes to the LED task.
- **Beat storage** on the 9.9 MB FAT partition of the flash
- **Serial console** at 115200 baud, for testing without WiFi (see [Serial console](#serial-console))

---

## Hardware

| Item | Details |
|------|---------|
| Board | **ESP32-S3-DevKitC-1 N16R8** (or a compatible clone): 16 MB flash, 8 MB octal (OPI) PSRAM |
| Touch | **GPIO1-GPIO14**, all 14 touch channels. GPIO1 = kick, GPIO2 = clap, GPIO3-GPIO14 = synth keys. Just touch the bare header pins with your finger. |
| RGB LED | Onboard addressable RGB LED (WS2812 type) on **GPIO48** (`RGB_BUILTIN`, driven with `rgbLedWrite()`) |
| Button | **BOOT = GPIO0** (reaction game, start/stop the beat) |
| USB | The USB-C port labelled **COM/UART** (USB-to-serial chip). It is used for uploads and for the serial console. |

**Nothing else is needed.** No breadboard, no wires, no extra parts.

> GPIO35-GPIO37 are used by the octal PSRAM on N16R8 modules, so this sketch does not use them.

---

## Software setup

### Arduino IDE

1. Install **Arduino IDE 2.x**.
2. In *Tools > Board > Boards Manager*, install **esp32 by Espressif Systems**, version **3.3.12**.
   If it is not listed, add this URL under *File > Preferences > Additional boards manager URLs*:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Open `esp32_lab/esp32_lab.ino`. The other files open as tabs.
4. Choose these settings in the *Tools* menu:

   | Setting | Value |
   |---------|-------|
   | Board | **ESP32S3 Dev Module** |
   | USB CDC On Boot | **Disabled** |
   | Flash Size | **16MB (128Mb)** |
   | PSRAM | **OPI PSRAM** |
   | Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |

   Leave the other settings at their defaults (CPU frequency 240 MHz, Arduino runs on core 1).
5. Select the COM port of the board and click **Upload**.

No extra libraries are needed. WiFi, WebServer, DNSServer, Preferences, FFat, BLE (NimBLE),
mbedTLS and esp-dsp all come with the esp32 core.

> Keep the folder name `esp32_lab`. The Arduino IDE needs the sketch folder to have the same name as the main `.ino` file.

### arduino-cli

FQBN:

```
esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB
```

(`CDCOnBoot=default` is the same as *USB CDC On Boot: Disabled*.)

```sh
# one-time setup
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json

# find the port of the board
arduino-cli board list

# compile, upload and open the serial monitor (run from the repository root)
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB esp32_lab
arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB esp32_lab
arduino-cli monitor -p COM5 -c baudrate=115200
```

Replace `COM5` with your own port. On Linux it is usually `/dev/ttyUSB0` or `/dev/ttyACM0`, and on
macOS `/dev/cu.usbserial-...` or `/dev/cu.wchusbserial...`.

After boot, the serial monitor (115200 baud) shows the chosen WiFi channel, the address of the
web app and whether the PSRAM was found.

---

## Usage

1. Upload the sketch. Do not touch the pins while the board starts, because the touch pins calibrate at boot.
2. On your phone, connect to the WiFi network **`ESP32-Lab`** with password **`12345678`**.
3. The captive portal should open the web app by itself. If it does not, open **http://192.168.4.1** in your browser. Any other `http://` address also redirects to the app.

Tips:
- **Tap "Turn on sound" first** on the Music tab. Browsers only allow audio after you tap something.
- **iPhone:** turn off silent mode, or Web Audio stays silent.
- **Phone speaker vs Bluetooth:** use the phone speaker or wired headphones when you play the drum pins. Bluetooth headphones add a delay that you can clearly hear (often well over 100 ms). With headphones or a good speaker you hear the bass best.
- The captive portal window is a limited mini browser. For the full experience (sound, live connection), open http://192.168.4.1 in Safari or Chrome. See [Troubleshooting](#troubleshooting).
- Watch the Dashboard while the Fractal or the Benchmark runs, to see how busy the cores get.

---

## Benchmark results

Measured on a real board: arduino-esp32 **3.3.12**, **240 MHz**, `-Os` build, with the compute
kernels (fractal and benchmark) built with `-O2` through `#pragma GCC optimize`.

| Test (`id`) | Result | Note |
|-------------|-------:|------|
| Integer multiply-add (`int`) | **102.5 million/s** | 4 independent chains |
| Float multiply-add, with FPU (`float`) | **39.0 million/s** | single-precision hardware FPU |
| Double multiply-add, no FPU (`double`) | **1.44 million/s** | done in software: **27x slower** than float |
| Float on 2 cores (`cores2`) | **77.6 million/s** | **1.99x** one core |
| Dot product, plain C (`plain`) | **16.5 million MAC/s** | `dsps_dotprod_f32_ansi`, 1024 floats |
| Dot product, esp-dsp SIMD (`simd`) | **134.8 million MAC/s** | `dsps_dotprod_f32_aes3`: 128-bit loads + 4 accumulators, **8.2x** plain C |
| `memcpy` in internal SRAM (`sram`) | **371.6 MB/s** | 32 KB blocks |
| `memcpy` in PSRAM (`psram`) | **21.9 MB/s** | 1 MB blocks (larger than the cache); internal SRAM is **17x** faster |
| Flash read (`flash`) | **12.6 MB/s** | `esp_partition_read`, 16 KB blocks from the FAT partition |
| SHA-256, hardware (`sha`) | **83.6 MB/s** | mbedTLS on the SHA accelerator |
| AES-128-CBC encrypt, hardware (`aes`) | **49.5 MB/s** | mbedTLS on the AES accelerator |
| True random numbers (`trng`) | **177.7 kB/s** | `esp_fill_random`, waits for fresh RF noise |

The `cores1` test (float on 1 core) runs the same code as `float`. It is the baseline for `cores2`.

Other measurements:

| What | Value |
|------|-------|
| Firmware size | 1.39 MB (44% of the 3 MB app partition) |
| Static RAM (global variables) | 59 KB |
| Free internal heap | ~196 KB at boot, ~124 KB after the first Bluetooth scan (NimBLE init) |
| CPU load at idle | core 0 ~1%, core 1 ~4% |
| Fractal | 400 x 300 pixels on 2 cores, with an atomic row counter |
| WiFi | at boot it picks the quietest of channels 1, 6 and 11; the access point allows up to 8 clients |
| FreeRTOS tasks | 16 in the task list: `loopTask`, `IDLE0`/`IDLE1`, `led`, `touch`, `live`, `fractal`, `wifi`, `tiT`, `async_udp`, `esp_timer`, `ipc0`/`ipc1`, `Tmr Svc`, `sys_evt`, `arduino_events` |

---

## Architecture

The work is split over the two cores. Core 0 runs the radio stacks and the light background
jobs. Core 1 runs everything that must react quickly to you.

| Task | Core | Priority | What it does |
|------|:----:|:--------:|--------------|
| `loopTask` (Arduino `loop()`) | 1 | 1 | HTTP web server on port 80, reaction game logic, system statistics once per second, serial console |
| `touch` | 1 | **3** | Reads the 14 touch pins and the BOOT button exactly every 10 ms (`vTaskDelayUntil`), and wakes the live task when something changes |
| `live` | 1 | 2 | WebSocket server on port 81. Sends the status every 40 ms, or right away when the touch task wakes it |
| `led` | 0 | 1 | LED effects, synth glow, beat flashes and the "green" moment of the reaction game, at 50 Hz |
| `fractal` | 0 | 1 | Helper that sleeps until it gets a notification, then computes fractal rows together with `loopTask` |
| WiFi, lwIP (`tiT`), NimBLE | 0 | system | The radio and network stacks |
| `storage` (short-lived) | 0 | 1 | Mounts the FAT partition at boot (formats it the first time), then deletes itself |
| `bench` (short-lived) | 0 | 1 | The core 0 half of the 2-core benchmark |

Why this layout:
- **Responsiveness.** On core 1 the touch task has the highest priority, so a long HTTP response never delays a touch reading. The live task comes next, so a touch reaches your phone before the web server continues. While `loopTask` is busy with a fractal or a benchmark, the touch and live tasks still interrupt it. That is why the dashboard keeps updating during heavy work.
- **The radio lives on core 0.** WiFi, lwIP and Bluetooth run on core 0. The jobs that must react quickly run on core 1, where the radio does not get in their way.
- **The task watchdog checks `IDLE0`.** In the Arduino core, the task watchdog checks that the idle task on core 0 gets to run now and then. If it does not run for 5 seconds, the watchdog can reset the chip. That is why the fractal helper on core 0 takes a 1 ms break every 500 ms. Core 1 needs no breaks, because its idle task is not checked.

Data flow:

```
 phone (browser)
   |-- HTTP :80 ...... page, API, /status fallback ......> loopTask   (core 1, prio 1)
   |-- WebSocket :81 . status 25x/s, pings, flashes .....> live       (core 1, prio 2)
   '-- DNS :53 ....... every name -> 192.168.4.1 ........> DNS server (captive portal)

 touch pins + BOOT --> touch (core 1, prio 3, every 10 ms) --notify--> live
 live --FreeRTOS queue (beat flashes)--> led (core 0, 50 Hz) --> RGB LED (GPIO48)
 loopTask --notify--> fractal helper (core 0) --notify when done--> loopTask
```

---

## How it works

### Hand-written WebSocket server
`live.ino` contains a small WebSocket server (RFC 6455) built on `WiFiServer`, without
an external library:
1. The browser sends a normal HTTP request with `Upgrade: websocket` and a random `Sec-WebSocket-Key`.
2. The ESP32 replies `101 Switching Protocols` with `Sec-WebSocket-Accept = base64(SHA-1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))`. It computes this with `mbedtls_sha1` and `mbedtls_base64_encode`.
3. After that, both sides send small frames. Incoming frames must be masked and complete (not split into parts). Their payload is unmasked with XOR.

Sending never blocks. Before each frame, `select()` with a zero timeout checks if the socket has
room, and `send(..., MSG_DONTWAIT)` sends the frame. If the phone is slow, that status message is
skipped (the next one follows 40 ms later). Half a frame would break the stream, so a partial
send closes the connection and the page reconnects. The server allows up to 4 clients and drops
a client that says nothing for 5 seconds. The page sends a ping every second, which also
measures the round-trip time. On the Music tab it also sends a tiny `k` message every 50 ms,
which keeps the phone's WiFi out of power-save mode, so pushed messages arrive without delay.

### Touch: hysteresis and a drifting baseline
On the ESP32-S3 the touch reading goes **up** when you touch a pin.
- At boot (and on *Recalibrate*), each pin's **baseline** is the average of 8 readings.
- Every 10 ms the touch task computes how many percent the reading is above the baseline. A pin counts as touched above **20%**, and as released only below **12%**. This **hysteresis** stops it from flickering around the threshold.
- A new touch within 40 ms of the previous one is treated as bounce and not counted.
- When a pin is *not* touched, the baseline slowly follows the reading. It moves down fast (5% of the difference per sample) and up very slowly (0.2%, only while the reading is less than 6% above the baseline). This keeps touch detection reliable when the temperature or humidity changes.
- Each pin has a tap counter and an "ms ago" value. With these the page never misses a short tap between two status messages, and it can work out when you actually touched a drum pin: *received - round-trip/2 - ago - ~25 ms sensor smoothing*.
- Calibration runs inside the touch task itself, so two tasks never read the sensors at the same time.
- The BOOT button is debounced: it must read the same 3 times in a row (30 ms). Every press is counted, so even a very short press starts or stops the beat.

### Fractal on two cores
- Both cores run the same function, `computeRows()`. Each core takes the next free row with `std::atomic<int>::fetch_add(1)` until all rows are done. Rows inside the set take much longer than rows outside it, so dividing the rows in advance would leave one core idle. With this dynamic split, both cores finish at about the same time.
- `loopTask` wakes the helper on core 0 with `xTaskNotifyGive`, computes rows itself, and then waits with `ulTaskNotifyTake` until the helper is done.
- **Cardioid and bulb test**: points inside the main cardioid, `q(q + (x - 1/4)) <= y^2/4` with `q = (x - 1/4)^2 + y^2`, or inside the period-2 bulb, `(x + 1)^2 + y^2 <= 1/16`, are black for sure. They are skipped without iterating. That saves a lot of work, because those points would otherwise run all the way to the iteration limit.
- The result is 1 byte per pixel in PSRAM (0 = inside the set, otherwise 1 + the escape step). The phone turns it into colors with a palette.
- This kernel is built with `-O2`. The rest of the sketch uses the default `-Os`.

### Beat-synced LED
- The page plans its audio ahead: every 25 ms it schedules the next 120 ms (1.6 s when the screen is off). So for every kick and clap it knows the exact future moment.
- For each hit it sends `f<ms>,<hue>,<strength>` over the WebSocket. `ms` is the time until the sound, plus the speaker output latency, minus half the round-trip time. The kick is turquoise (hue 165) and the clap is pink (hue 320).
- The live task calls `ledFlash()`, which puts `{time, hue, strength}` into a **FreeRTOS queue** (16 places, never waits; if the queue is full, the flash is skipped). The queue is a safe way to pass data from core 1 to core 0 without locks.
- Every 20 ms the LED task moves new flashes from the queue to its own list, fires each one when `millis()` reaches its time, and lets it fade quickly (x0.8 per frame).
- If several things want the LED at once, the most important one wins: reaction game > synth glow > beat flash > chosen effect.

### CPU load from FreeRTOS run-time stats
Once per second `uxTaskGetSystemState()` returns, for every task, how many microseconds it
has run in total. The difference with the previous second gives the CPU % per task. Each core
also has an idle task that only runs when the core has nothing else to do. So
**load = 100% - share of the idle task** (`xTaskGetIdleTaskHandleForCore()`). The same snapshot
gives the core, priority and stack high-water mark for the task list.

### Quietest WiFi channel
Before the access point starts, the ESP32 does a quick scan (hidden networks included). Each
network adds a score to channels 1, 6 and 11. The score depends on its signal strength
(`RSSI + 100`, limited to 1-70) and on how close its channel is: `(5 - distance) / 5`, for
channels less than 5 apart, because neighboring channels overlap. The channel with the lowest
total wins. The access point then starts on that channel with room for 8 clients. WiFi stays
in AP+STA mode, so the WiFi scanner can still scan later.

### Smaller details
- **Faster web server**: `LabServer` extends `WebServer`. Some browsers open an extra "for later" connection that never sends anything, and the standard server waits up to 5 s for it. `LabServer` closes such a connection after 500 ms, and turns on `setNoDelay` so small replies go out right away.
- **Page caching**: the page is about 100 KB, so it is sent with an `ETag` based on the build date and time. A browser that already has the page gets `304 Not Modified`.
- **Safe beat saving**: a beat is first written to a `.tmp` file and then renamed. If the power fails halfway, the old beat stays intact.

---

## HTTP API and WebSocket reference

All endpoints are on `http://192.168.4.1` (port 80). Any unknown path is redirected (`302`) to
`http://192.168.4.1/`. This makes the captive portal work.

### Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | The web app (`ETag`, `304` when unchanged) |
| GET | `/status` | Live status as JSON (see below) |
| GET | `/info` | Fixed chip information as JSON |
| GET | `/led?color=rrggbb&brightness=0-255&effect=0-7` | Set the LED. All parameters are optional. Returns `ok` |
| GET | `/game/start` | Start a reaction game round. Returns `ok` |
| GET | `/touch/calibrate` | Recalibrate the touch pins (hands off!). `200` when done, `503` if it took longer than 3 s |
| GET | `/fractal?x=&y=&s=&i=&c=` | Compute the fractal (binary reply, see below) |
| GET | `/scan` | WiFi scan (poll until `busy` is `false`) |
| GET | `/ble` | Bluetooth LE scan (poll until `busy` is `false`) |
| GET | `/benchmark?t=N` | Run benchmark test `N` (0-12) |
| GET | `/tasks` | Load per core and the FreeRTOS task list |
| GET | `/beat/list` | Which beat slots are in use |
| POST | `/beat/save?kit=deep\|afro&slot=1-4` | Save a beat. The body is the JSON beat (max 8000 bytes) |
| GET | `/beat/load?kit=deep\|afro&slot=1-4` | Load a beat (the saved JSON, unchanged) |
| GET | `/restart` | Replies `ok`, then restarts the ESP32 |
| GET | `/favicon.ico` | `204 No Content` |

### `/status`
The same JSON is pushed over the WebSocket.

| Key | Meaning |
|-----|---------|
| `temp` | Chip temperature in °C |
| `uptime` | Seconds since boot |
| `heap`, `heapMin` | Free internal heap now / lowest since boot (bytes) |
| `psram` | Free PSRAM (bytes) |
| `devices` | Number of devices on the access point |
| `rssi` | Their signal strengths in dBm (max 10) |
| `requests` | HTTP requests in the last second |
| `live` | Number of open WebSocket connections |
| `cpu` | `[core0, core1]` load in %. Missing until the first measurement |
| `button` | `true` while BOOT is held down |
| `buttonCount` | Number of BOOT presses since boot |
| `effect`, `color`, `brightness` | LED settings (`color` as `"rrggbb"`) |
| `touch` | 14 values: % above the baseline per pin |
| `touched` | 14 values: `1` = touched, `0` = not touched |
| `taps` | 14 tap counters |
| `ago` | 14 values: ms since the last touch (max 9999) |
| `game` | Game state: 0 ready, 1 waiting (red), 2 now (green), 3 result, 4 too early, 5 too late |
| `reaction`, `record` | Last reaction time and the record, in ms (`record` 0 = none yet) |
| `newRecord` | `true` if the last round set a new record |

### `/info`
`chip`, `revision`, `cores`, `mhz`, `flash` (bytes), `ram` (heap size), `psramTotal`, `mac`,
`sdk` (ESP-IDF version), `wifi` (network name), `channel`, `fractalW`, `fractalH`,
`storageTotal`, `storageFree` (FAT partition, bytes), `resetReason` (text), `touchPins`
(array of GPIO numbers, in key order).

### `/fractal`
- Parameters: `x`, `y` = center; `s` = width of the view in the complex plane (0 < s <= 10, default 3.2); `i` = iteration limit (10-255); `c=1` uses one core, any other value uses two.
- Reply: `application/octet-stream`, `fractalW x fractalH` bytes, row by row. `0` = inside the set, otherwise 1 + the escape step.
- Headers: `X-Time` = computation time in ms, `X-Iter` = total number of iteration steps.

### `/scan` and `/ble`
```json
{"busy": true}
{"busy": false, "networks": [{"name": "...", "rssi": -60, "channel": 6, "open": false}]}
{"busy": false, "devices":  [{"name": "...", "rssi": -70, "maker": "Apple"}]}
{"error": "..."}
```
The first call starts the scan in the background. Later calls return `{"busy":true}` until the
results are ready. `/ble` returns `error` if Bluetooth could not start.

### `/benchmark?t=N`
```json
{"t": 6, "id": "simd", "name": "...", "unit": "million/s", "group": "simd", "value": 134.80, "last": false}
```
- ids: `int`, `float`, `double`, `cores1`, `cores2`, `plain`, `simd`, `sram`, `psram`, `flash`, `sha`, `aes`, `trng`
- groups: `compute`, `cores`, `simd`, `memory`, `hardware`
- units: `million/s`, `MB/s`, `kB/s`
- A test number out of range returns `404` with `{"error":"no test"}`.

### `/tasks`
```json
{"cpu": [1.2, 4.0], "tasks": [{"name": "loopTask", "core": 1, "prio": 1, "stack": 5000, "cpu": 3.2}]}
```
`core` is `-1` for tasks that may run on both cores. `stack` is the stack high-water mark: the number of bytes that have never been used.

### Beat storage
- `/beat/list` returns `{"deep":[true,false,false,false],"afro":[false,false,false,false]}`.
- `/beat/save` returns `200 ok`. It returns `400` for a bad kit, slot or body, `503` while the storage is still starting, and `500` for a write error.
- `/beat/load` returns `404` with `{"error":"empty"}` when the slot is empty (or the kit or slot is not valid).
- Files are stored as `/beats/<kit>-<n>.json` on the FAT partition, and the ESP32 saves them unchanged. The page writes:
  ```json
  {"version": 1, "set": "...", "patterns": [[...], [...], [...], [...]], "bpm": 118, "swing": 20, "progression": "...", "key": "A minor"}
  ```
  The page also reads beats saved by the earlier Dutch version of this project (keys `patronen`, `progressie`, `toon`).
- The reaction game record is stored separately, in NVS (`Preferences` namespace `lab`, key `record`).

### WebSocket (`ws://192.168.4.1:81/`)

| Direction | Message | Meaning |
|-----------|---------|---------|
| ESP32 -> page | status JSON | Same as `/status`. Sent every 40 ms (25x/s), and right away after a touch or BOOT change |
| page -> ESP32 | `p<anything>` | Ping. Sent back unchanged (the page uses it to measure the round-trip time) |
| page -> ESP32 | `f<ms>,<hue>,<strength>` | Flash the LED in `ms` ms (0-3000), with hue 0-359 and strength 0-255 (optional, default 255) |
| page -> ESP32 | `k` | Keep-awake. Ignored, but keeps the phone's WiFi and the connection active |

Limits: at most 4 clients. Text frames up to 200 bytes, masked and not split into parts. A
client that is silent for 5 s is disconnected. Control frames: ping gets a pong, close gets a
close.

---

## Serial console

Open the serial monitor at **115200 baud** and type a command. Commands are not case-sensitive.
This is useful for testing without WiFi.

| Command | What it does |
|---------|--------------|
| `help` (or `?`) | List of commands |
| `status` | Temperature, uptime, free RAM (and the lowest ever), free PSRAM, load per core, WiFi devices, live connections, requests per second, storage, reason for the last restart |
| `tasks` | All FreeRTOS tasks with core, priority, CPU % and free stack |
| `touch` | Reading, baseline, % difference and touch state of each touch pin |
| `benchmark` | Run all 13 speed tests (takes a few seconds) |
| `wifi` | Scan for nearby WiFi networks |
| `ble` | Scan for Bluetooth LE devices (5 seconds) |
| `restart` | Restart the ESP32 |

The console runs in `loop()`, so the web server pauses while a long command (`benchmark`,
`wifi`, `ble`) runs.

---

## Files

```
esp32-s3-lab/
|-- README.md
|-- LICENSE
|-- .gitignore
'-- esp32_lab/              Arduino sketch folder (open esp32_lab.ino)
    |-- esp32_lab.ino       main tab: settings, LED task, touch task, reaction game,
    |                       fractal, web server routes, setup() and loop()
    |-- benchmark.ino       the 13 speed tests and /benchmark
    |-- bluetooth.ino       BLE scanner (NimBLE) and /ble
    |-- live.ino            the hand-written WebSocket server on port 81
    |-- storage.ino         beat storage on the FAT partition: /beat/list, /beat/save, /beat/load
    |-- system.ino          CPU load, task list (/tasks), connected devices, reset reason,
    |                       quietest-channel selection and the serial console
    '-- webpage.h           the whole web app (HTML, CSS, JavaScript) as one PROGMEM string
```

The Arduino IDE joins the tabs together: first the main `.ino`, then the others in
alphabetical order. All source files are plain ASCII.

---

## Troubleshooting

**Upload fails: "port busy", "access denied" or "could not open port"**
Only one program can use the COM port at a time. Close the Serial Monitor in the Arduino IDE
(or stop `arduino-cli monitor`) before you upload. If the upload still does not start, hold
**BOOT**, tap **RESET**, release BOOT and try again.

**No serial output**
Use the USB-C port labelled **COM/UART**, and set *USB CDC On Boot* to **Disabled**. With that
setting, `Serial` goes to the UART port and not to the native USB port.

**The first boot takes longer / beat storage says "not available"**
On the first boot (or after a change of partition scheme), the 9.9 MB FAT partition is
formatted. This runs in a background task, so the web app starts right away, but saving beats
only works once the formatting is done (watch the serial monitor). If storage never becomes
ready, check that the partition scheme is **16M Flash (3MB APP/9.9MB FATFS)**. Without a FAT
partition, the flash benchmark also shows 0.

**PSRAM "not found"**
Set *PSRAM* to **OPI PSRAM**. Without PSRAM the fractal falls back to 200 x 150 pixels and the
PSRAM benchmark shows 0.

**The captive portal window is limited**
The captive portal pop-up on phones is a mini browser. It may block sound, close when you
switch apps, or drop the live connection. Close it, choose to stay connected or use the network
without internet, and open **http://192.168.4.1** in Safari or Chrome.

**Android disconnects or the page does not load**
Android may warn that the network has no internet access. Choose **Stay connected** (or "Keep
WiFi connection"). If the phone keeps switching to mobile data, turn off mobile data or the
automatic network switching for a moment.

**No sound**
Tap **Turn on sound** on the Music tab, turn off silent mode on an iPhone, and check the volume
slider and the phone volume.

**Drums feel late**
Bluetooth headphones add latency. Use the phone speaker or wired headphones.

**Touch pins react strangely (stuck on, or no response)**
Do not touch the pins while the board starts. Later, take your hands off the board and tap
**Recalibrate** on the Music tab. The `touch` console command shows the reading, baseline and
% difference of each pin.

**The connection hiccups during a WiFi or Bluetooth scan**
This is normal. WiFi and Bluetooth share one radio, and while scanning the ESP32 briefly
listens on other channels.

**The onboard LED stays dark**
Many N16R8 boards have the RGB LED on GPIO48. Some board revisions (for example
DevKitC-1 v1.1) use GPIO38. In that case, replace `RGB_BUILTIN` with `38` in `esp32_lab.ino`.

---

## License

This project is released under the [MIT License](LICENSE). You may use, change and share the
code, as long as you keep the copyright notice and the license text.
