/*
  =====================================================================
   ESP32-S3 LAB  -  shows everything your board can do
  =====================================================================
   All you need is the board and your phone (or laptop).

   What is inside?
   - Its own WiFi network with a "captive portal": the page opens by itself
   - Live connection (WebSocket): the ESP32 itself sends what happens, right away
   - Dashboard with live charts: temperature, memory, load per core
   - LED studio: color picker, brightness and 8 effects (running on core 0)
   - Music studio: deep house and afro house with drums, bass and chords,
     random variations, live buttons, DJ effects and a synth on the
     touch pins. The LED flashes along with the beat, and you can save
     beats in the flash memory.
   - Reaction game with the BOOT button (interrupt + record in flash memory)
   - Fractal explorer: the ESP32 calculates a Mandelbrot figure,
     on 1 or on 2 cores at the same time
   - Benchmark: computing, memory, vector instructions and special hardware
   - WiFi scanner and Bluetooth scanner
   - Task list: which tasks are running, and on which core?
   - Serial console: type "help" in the Serial Monitor (115200 baud)

   The code is split over several tabs (files):
     esp32_lab.ino   this file: LED, touch, game, fractal, web server
     live.ino        the live connection (WebSocket)
     benchmark.ino   the speed tests
     bluetooth.ino   the Bluetooth scanner
     storage.ino     saving beats in the flash memory
     system.ino      load per core, task list and the serial console
     webpage.h       the web app: HTML, CSS and JavaScript

   Settings in the Arduino IDE (Tools menu):
     Board: ESP32S3 Dev Module        USB CDC On Boot: Disabled
     Flash Size: 16MB (128Mb)         PSRAM: OPI PSRAM
     Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)

   How to use:
     1. Upload this sketch.
     2. Connect your phone to the WiFi "ESP32-Lab" (password: 12345678).
     3. The page opens by itself. If not: go to http://192.168.4.1
  =====================================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <atomic>
#include "webpage.h"     // the web app: HTML, CSS and JavaScript (second tab)

// ============================ SETTINGS ================================
const char* WIFI_NAME       = "ESP32-Lab";
const char* WIFI_PASSWORD   = "12345678";      // at least 8 characters

const int BUTTON_PIN = 0;                       // the BOOT button
// All 14 touch pins of the ESP32-S3. Pin 1 and 2 (right row) are the drums
// (kick and clap), the rest are in the same order as they sit in the left row.
const int TOUCH_COUNT = 14;
const int TOUCH_PINS[TOUCH_COUNT] = {1, 2, 4, 5, 6, 7, 8, 3, 9, 10, 11, 12, 13, 14};
const int TOUCH_THRESHOLD = 20;                 // % difference that counts as a touch
const int TOUCH_RELEASE = 12;                   // only below this % does it count as released

// ============================ OBJECTS =================================
// The normal WebServer only handles 1 connection at a time. Some browsers already open
// an extra connection "for later" on which nothing arrives; then the server waits up to 5 seconds.
// Our own version waits half a second at most, and sends small packets right away.
class LabServer : public WebServer {
public:
  using WebServer::WebServer;
  void handleClientFast() {
    if (_currentStatus == HC_WAIT_READ && !_currentClient.available() && millis() - _statusChange > 500) {
      _currentClient.stop();                    // empty connection: get rid of it
      _currentStatus = HC_NONE;
    }
    handleClient();
  }
  void enableNoDelay() { _server.setNoDelay(true); }
};
LabServer server(80);          // web server on port 80
DNSServer dnsServer;           // sends every web address to the ESP32
Preferences prefs;             // keeps small data (the record) in the flash memory

// ============================ LED =====================================
// Effects: 0=off 1=solid 2=breathe 3=rainbow 4=disco 5=fire 6=police 7=temperature
// "volatile" = this variable is used by two cores, so always read it fresh
volatile int ledEffect = 2;
volatile int ledR = 0, ledG = 255, ledB = 100;  // chosen color (turquoise)
volatile int ledBrightness = 60;                // 0 to 255

// LED flashes on the beat: the web page tells us in advance when the kick or clap will sound.
// A queue safely carries those messages from the live task to the LED task.
struct BeatFlash {
  unsigned long time;                           // millis() at which the LED must flash
  int hue;                                      // position on the color wheel (0-359)
  int strength;                                 // 0-255
};
QueueHandle_t flashQueue = nullptr;

// ============================ REACTION GAME ===========================
enum GameState { GAME_READY, GAME_WAITING, GAME_NOW, GAME_RESULT, GAME_TOO_EARLY, GAME_TOO_LATE };
volatile int gameState = GAME_READY;
volatile unsigned long gameGreenAt = 0;         // millis() at which the LED turns green
volatile unsigned long greenMicros = 0;         // exact moment the LED turned green
volatile bool buttonPressed = false;            // is set by the interrupt
volatile unsigned long buttonMicros = 0;        // exact moment of the press
unsigned long reactionMs = 0;
unsigned long recordMs = 0;
bool newRecord = false;

// ============================ TOUCH ===================================
float touchBaseline[TOUCH_COUNT];               // value without touch (slowly follows along)
uint32_t touchValue[TOUCH_COUNT];               // latest reading
volatile bool touchActive[TOUCH_COUNT];         // is the pin being touched right now?
volatile uint32_t touchTaps[TOUCH_COUNT];       // number of touches (so your phone never misses a tap)
volatile unsigned long touchStartMs[TOUCH_COUNT];   // millis() of the latest touch (for the drums)
volatile bool calibrateRequest = false;         // the web page asks: calibrate again
volatile uint32_t buttonCount = 0;              // how often BOOT was pressed (for the music)

// ============================ FRACTAL =================================
int fractalW = 400, fractalH = 300;             // resolution (smaller without PSRAM)
uint8_t* fractalPixels = nullptr;               // the result: 1 byte per pixel
volatile float fCx = 0, fCy = 0, fStep = 0;     // center and step size
volatile int fMaxIter = 100;                    // maximum number of repeats
std::atomic<int> nextRow(0);                    // the next row that nobody is computing yet
std::atomic<uint32_t> fractalSteps(0);          // how many compute steps there were in total
TaskHandle_t fractalTaskHandle = nullptr;
TaskHandle_t webTaskHandle = nullptr;

// ============================ SYSTEM ==================================
float temperature = 0;
unsigned long requestCounter = 0, requestsPerSecond = 0;
unsigned long lastSecond = 0;
float cpuLoad[2] = {-1, -1};                    // % busy per core (-1 = not known yet)
int wifiChannel = 1;                            // the WiFi channel of our network
int deviceCount = 0;                            // phones/laptops on our WiFi network
int deviceRssi[10];                             // their signal strength in dBm
int rssiCount = 0;


// =====================================================================
//  HELPER FUNCTION: turn the color wheel (0-359 degrees) into red, green, blue
// =====================================================================
void colorWheel(int angle, int &r, int &g, int &b) {
  angle = angle % 360;
  int part = angle / 60;                 // which sixth of the wheel
  int f = (angle % 60) * 255 / 60;       // how far inside that part (0-255)
  switch (part) {
    case 0:  r = 255;     g = f;       b = 0;       break;
    case 1:  r = 255 - f; g = 255;     b = 0;       break;
    case 2:  r = 0;       g = 255;     b = f;       break;
    case 3:  r = 0;       g = 255 - f; b = 255;     break;
    case 4:  r = f;       g = 0;       b = 255;     break;
    default: r = 255;     g = 0;       b = 255 - f; break;
  }
}

// Called by the live connection: "flash in ms milliseconds"
void ledFlash(int ms, int hue, int strength) {
  if (flashQueue == nullptr || ms < 0 || ms > 3000) return;
  BeatFlash f = { millis() + ms, ((hue % 360) + 360) % 360, constrain(strength, 0, 255) };
  xQueueSend(flashQueue, &f, 0);                // queue full? then skip this one, do not wait
}


// =====================================================================
//  LED TASK  -  runs separately on core 0, 50 times per second
// =====================================================================
void ledTask(void* parameter) {
  int fire = 180;
  unsigned long nextDisco = 0;
  int discoR = 0, discoG = 0, discoB = 0;
  float glow[TOUCH_COUNT] = {0};                // how bright each synth key is still glowing
  BeatFlash scheduled[16];                      // flashes that are still to come
  int scheduledCount = 0;
  float flashLevel = 0;                         // how bright the beat flash still is (0..1)
  int flashHue = 0;
  unsigned long lastFlash = 0;                  // millis() of the latest beat flash

  while (true) {
    unsigned long now = millis();
    int r = 0, g = 0, b = 0;
    int brightness = ledBrightness;

    // Synth: every touched pin makes its own color light up and slowly fade out
    float glowR = 0, glowG = 0, glowB = 0, glowTotal = 0;
    for (int p = 0; p < TOUCH_COUNT; p++) {
      if (touchActive[p]) glow[p] = (glow[p] > 0.88f) ? 1.0f : glow[p] + 0.12f;   // quickly on
      else glow[p] = glow[p] * 0.95f;                                            // slowly off
      if (glow[p] > 0.01f) {
        int kr, kg, kb;
        colorWheel(p * 360 / TOUCH_COUNT, kr, kg, kb);
        glowR += kr * glow[p];
        glowG += kg * glow[p];
        glowB += kb * glow[p];
        glowTotal += glow[p];
      }
    }

    // Beat: take new flashes from the queue, and check if it is time
    BeatFlash f;
    while (scheduledCount < 16 && xQueueReceive(flashQueue, &f, 0) == pdTRUE) scheduled[scheduledCount++] = f;
    for (int i = 0; i < scheduledCount; ) {
      if ((long)(now - scheduled[i].time) >= 0) {   // time to flash!
        flashLevel = scheduled[i].strength / 255.0f;
        flashHue = scheduled[i].hue;
        lastFlash = now;
        scheduled[i] = scheduled[--scheduledCount]; // remove it from the list
      } else {
        i++;
      }
    }

    // Reaction game: is it time to turn green?
    if (gameState == GAME_WAITING && now >= gameGreenAt && !buttonPressed) {
      rgbLedWrite(RGB_BUILTIN, 0, 150, 0);      // green right away
      greenMicros = micros();                    // the reaction time starts now
      gameState = GAME_NOW;
    }

    if (gameState == GAME_NOW) {
      // LED stays green until the button is pressed: do nothing
    } else if (gameState == GAME_WAITING) {
      r = 255;                                   // red: wait...
      brightness = 120;
    } else if (glowTotal > 0.01f) {
      r = constrain((int)glowR, 0, 255);         // synth: mix the colors of the keys
      g = constrain((int)glowG, 0, 255);
      b = constrain((int)glowB, 0, 255);
    } else if (now - lastFlash < 1500) {         // the beat is playing: flash in the color of kick or clap
      colorWheel(flashHue, r, g, b);
      r *= flashLevel; g *= flashLevel; b *= flashLevel;
    } else {
      switch (ledEffect) {
        case 1:   // solid
          r = ledR; g = ledG; b = ledB;
          break;

        case 2: { // breathe: soft wave of 3 seconds
          float wave = (1.0f - cosf(now * 2.0f * PI / 3000.0f)) / 2.0f;   // 0..1
          wave = wave * wave;                    // feels more natural to your eyes
          r = ledR * wave; g = ledG * wave; b = ledB * wave;
          break;
        }

        case 3:   // rainbow: one round every 6 seconds
          colorWheel((now / 17) % 360, r, g, b);
          break;

        case 4:   // disco: bright color at random moments
          if (now >= nextDisco) {
            colorWheel(random(0, 360), discoR, discoG, discoB);
            nextDisco = now + random(60, 250);
          }
          r = discoR; g = discoG; b = discoB;
          break;

        case 5:   // fire: orange that flickers restlessly
          fire = constrain(fire + random(-45, 46), 60, 255);
          r = fire;
          g = fire * random(20, 40) / 100;
          break;

        case 6: { // police: two times red, two times blue
          int t = now % 1000;
          if (t < 80 || (t >= 160 && t < 240)) r = 255;
          if ((t >= 500 && t < 580) || (t >= 660 && t < 740)) b = 255;
          break;
        }

        case 7: { // temperature of the chip: blue (cool) to red (warm)
          int celsius = constrain((int)temperature, 30, 60);
          colorWheel(map(celsius, 30, 60, 240, 0), r, g, b);
          break;
        }

        default:  // 0 = off
          break;
      }
    }
    flashLevel *= 0.8f;                          // the flash fades out quickly

    if (gameState != GAME_NOW) {
      rgbLedWrite(RGB_BUILTIN, r * brightness / 255, g * brightness / 255, b * brightness / 255);
    }
    vTaskDelay(pdMS_TO_TICKS(20));               // sleep 20 ms = 50 times per second
  }
}


// =====================================================================
//  FRACTAL  -  calculating the Mandelbrot figure
// =====================================================================
// Both cores run this function at the same time. Each time they take the next
// free row, until all rows are done. This way no core finishes earlier than the other.
// The #pragma lines ask the compiler to make this one function extra fast (O2).
#pragma GCC push_options
#pragma GCC optimize("O2")
void computeRows() {
  const float cx = fCx, cy = fCy, d = fStep;    // copy: faster than volatile
  const int maxIter = fMaxIter;
  uint32_t steps = 0;
  unsigned long lastPause = millis();

  while (true) {
    int y = nextRow.fetch_add(1);               // "atomic": two cores never get the same row
    if (y >= fractalH) break;                   // everything is done
    // On core 0 a watchdog checks if the "idle" task still gets a turn now and then.
    // After 5 seconds without a turn it restarts the chip. So: every half second
    // a very short (1 ms) pause, then the watchdog is happy.
    if (xPortGetCoreID() == 0 && millis() - lastPause > 500) {
      vTaskDelay(1);
      lastPause = millis();
    }
    float ci = cy + (y - fractalH / 2) * d;
    uint8_t* row = fractalPixels + y * fractalW;
    for (int x = 0; x < fractalW; x++) {
      float cr = cx + (x - fractalW / 2) * d;
      // Clever trick: if the point lies in the big "heart" or in the circle to the left of it,
      // we know for sure it will be black. That saves a lot of computing.
      float q = (cr - 0.25f) * (cr - 0.25f) + ci * ci;
      if (q * (q + (cr - 0.25f)) <= 0.25f * ci * ci || (cr + 1.0f) * (cr + 1.0f) + ci * ci <= 0.0625f) {
        row[x] = 0;
        continue;
      }
      float zr = 0, zi = 0;
      int i = 0;
      // repeat z = z*z + c until z gets too big (or until maxIter)
      while (i < maxIter && zr * zr + zi * zi < 4.0f) {
        float newZr = zr * zr - zi * zi + cr;
        zi = 2.0f * zr * zi + ci;
        zr = newZr;
        i++;
      }
      steps += i;
      // 0 = inside the figure (black), otherwise: after how many steps it escapes
      row[x] = (i >= maxIter) ? 0 : (uint8_t)(1 + i);
    }
  }
  fractalSteps += steps;
}
#pragma GCC pop_options

// Helper task on core 0: waits for a signal and then helps with the computing
void fractalTask(void* parameter) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    // sleep until a signal arrives
    computeRows();
    xTaskNotifyGive(webTaskHandle);             // signal back: done!
  }
}


// =====================================================================
//  TOUCH: the 14 keys of the synth (own task, 100 times per second)
// =====================================================================
void touchCalibrate() {
  for (int p = 0; p < TOUCH_COUNT; p++) touchRead(TOUCH_PINS[p]);    // first switch on all sensors
  delay(300);                                                         // let them measure for a moment
  uint32_t sum[TOUCH_COUNT] = {0};
  for (int n = 0; n < 8; n++) {                 // 8 readings per pin, with some time in between
    for (int p = 0; p < TOUCH_COUNT; p++) sum[p] += touchRead(TOUCH_PINS[p]);
    delay(25);
  }
  for (int p = 0; p < TOUCH_COUNT; p++) {
    touchBaseline[p] = sum[p] / 8.0f;
    touchActive[p] = false;
  }
}

// How many percent is the latest reading ABOVE the baseline?
// (On the ESP32-S3 the reading only goes up when you touch a pin.)
int touchPercent(int p) {
  if (touchBaseline[p] < 1 || touchValue[p] <= touchBaseline[p]) return 0;
  return (int)(((float)touchValue[p] - touchBaseline[p]) * 100.0f / touchBaseline[p]);
}

// "Debouncing" the BOOT button: a button bounces for a moment when you press it. Only when
// it reads the same 3 times in a row (30 ms) do we believe it.
bool buttonUpdate() {
  static int stable = HIGH, count = 0;
  int now = digitalRead(BUTTON_PIN);
  if (now == stable) { count = 0; return false; }
  if (++count < 3) return false;
  stable = now;
  count = 0;
  if (now == LOW) buttonCount++;                // pressed: count it
  return true;                                  // pressed or released: pass it on right away
}

void touchUpdate() {
  bool changed = false;
  unsigned long now = millis();
  for (int p = 0; p < TOUCH_COUNT; p++) {
    touchValue[p] = touchRead(TOUCH_PINS[p]);
    int percent = touchPercent(p);
    // Touched above 20%, released again only below 12%: this way it does not flicker around the limit
    bool active = touchActive[p] ? percent > TOUCH_RELEASE : percent > TOUCH_THRESHOLD;
    if (active && !touchActive[p] && now - touchStartMs[p] > 40) {   // again within 40 ms = bounce, do not count
      touchTaps[p]++;                           // count the new touch
      touchStartMs[p] = now;                    // and remember exactly when
    }
    if (active != touchActive[p]) changed = true;
    touchActive[p] = active;
    // Nobody touching the pin? Then the baseline moves along with the reading.
    // This way it keeps working well when it gets warmer or more humid.
    if (!active) {
      float diff = (float)touchValue[p] - touchBaseline[p];
      if (diff < 0) touchBaseline[p] += diff * 0.05f;                // lower: follow quickly
      else if (percent < 6) touchBaseline[p] += diff * 0.002f;       // a bit higher: very slowly
    }
  }
  if (buttonUpdate()) changed = true;
  if (changed) liveNotify();                    // send it to your phone right away
}

void touchTask(void* parameter) {
  TickType_t nextWake = xTaskGetTickCount();
  while (true) {
    if (calibrateRequest) {                     // the web page asked to calibrate again
      touchCalibrate();
      calibrateRequest = false;
      nextWake = xTaskGetTickCount();
    }
    touchUpdate();
    vTaskDelayUntil(&nextWake, pdMS_TO_TICKS(10));   // exactly every 10 ms
  }
}


// =====================================================================
//  REACTION GAME
// =====================================================================
// This function is called by the hardware as soon as you press BOOT.
// IRAM_ATTR = put it in the fast working memory (required for interrupts).
void IRAM_ATTR buttonInterrupt() {
  if (!buttonPressed) {
    buttonMicros = micros();
    buttonPressed = true;
  }
}

void gameStart() {
  buttonPressed = false;
  newRecord = false;
  gameGreenAt = millis() + random(1500, 4500);  // wait 1.5 to 4.5 seconds
  gameState = GAME_WAITING;
}

void gameUpdate() {
  if (gameState == GAME_WAITING && buttonPressed) {
    gameState = GAME_TOO_EARLY;                 // pressed before it turned green
  }
  if (gameState == GAME_NOW) {
    if (buttonPressed) {
      long diff = (long)(buttonMicros - greenMicros);     // this way it is also correct when micros() starts again at 0
      if (diff < 100000L) {                     // faster than 0.1 s is impossible for a human: that is guessing
        gameState = GAME_TOO_EARLY;
      } else {
        reactionMs = diff / 1000;
        if (recordMs == 0 || reactionMs < recordMs) {
          recordMs = reactionMs;
          prefs.putUInt("record", recordMs);    // keep it, also after a restart
          newRecord = true;
        }
        gameState = GAME_RESULT;
      }
    } else if ((long)(micros() - greenMicros) > 3000000L) {
      gameState = GAME_TOO_LATE;                // 3 seconds without doing anything
    }
  }
}


// =====================================================================
//  WEB PAGES
// =====================================================================
// Makes text safe for JSON (e.g. a WiFi name with a " in it)
String jsonText(const String& text) {
  String out = "";
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((unsigned char)c < 32) { out += ' '; }
    else { out += c; }
  }
  return out;
}

// The web page is over 100 kB. If your browser already has it (the same version), we only send
// "304: nothing changed". We recognize the version by the moment it was compiled (the "ETag").
const char* PAGE_VERSION = "\"" __DATE__ " " __TIME__ "\"";

void handleRoot() {
  requestCounter++;
  server.sendHeader("Cache-Control", "no-cache");      // always ask whether there is a new version
  server.sendHeader("ETag", PAGE_VERSION);
  if (server.header("If-None-Match") == PAGE_VERSION) {
    server.send(304);
    return;
  }
  server.send_P(200, "text/html", WEBPAGE);
}

// All live data at once, as JSON. Used by /status
// and by the live connection (which sends this 25 times per second).
String makeStatusJson() {
  char color[8];
  snprintf(color, sizeof(color), "%02x%02x%02x", (int)ledR, (int)ledG, (int)ledB);
  unsigned long now = millis();

  String j;
  j.reserve(1000);
  j = "{";
  j += "\"temp\":" + String(temperature, 1);
  j += ",\"uptime\":" + String(now / 1000);
  j += ",\"heap\":" + String(ESP.getFreeHeap());
  j += ",\"heapMin\":" + String(ESP.getMinFreeHeap());
  j += ",\"psram\":" + String(ESP.getFreePsram());
  j += ",\"devices\":" + String(deviceCount);
  j += ",\"rssi\":[";
  for (int i = 0; i < rssiCount; i++) {
    if (i > 0) j += ",";
    j += String(deviceRssi[i]);
  }
  j += "],\"requests\":" + String(requestsPerSecond);
  j += ",\"live\":" + String(liveConnected());
  if (cpuLoad[0] >= 0) {
    j += ",\"cpu\":[" + String(cpuLoad[0], 1) + "," + String(cpuLoad[1], 1) + "]";
  }
  j += ",\"button\":";
  j += (digitalRead(BUTTON_PIN) == LOW) ? "true" : "false";
  j += ",\"buttonCount\":" + String(buttonCount);
  j += ",\"effect\":" + String(ledEffect);
  j += ",\"color\":\"" + String(color) + "\"";
  j += ",\"brightness\":" + String(ledBrightness);
  j += ",\"touch\":[";
  for (int p = 0; p < TOUCH_COUNT; p++) {
    if (p > 0) j += ",";
    j += String(touchPercent(p));
  }
  j += "],\"touched\":[";
  for (int p = 0; p < TOUCH_COUNT; p++) {
    if (p > 0) j += ",";
    j += touchActive[p] ? "1" : "0";
  }
  j += "],\"taps\":[";
  for (int p = 0; p < TOUCH_COUNT; p++) {
    if (p > 0) j += ",";
    j += String(touchTaps[p]);
  }
  j += "],\"ago\":[";                           // how many ms ago each pin was touched
  for (int p = 0; p < TOUCH_COUNT; p++) {
    if (p > 0) j += ",";
    long ago = (long)(now - touchStartMs[p]);   // negative = the touch task just stored a new tap
    if (ago < 0) ago = 0;
    j += String(ago > 9999 ? 9999L : ago);
  }
  j += "],\"game\":" + String(gameState);
  j += ",\"reaction\":" + String(reactionMs);
  j += ",\"record\":" + String(recordMs);
  j += ",\"newRecord\":";
  j += newRecord ? "true" : "false";
  j += "}";
  return j;
}

void handleStatus() {
  requestCounter++;
  server.send(200, "application/json", makeStatusJson());
}

// Fixed data about the chip
void handleInfo() {
  requestCounter++;
  String j = "{";
  j += "\"chip\":\"" + String(ESP.getChipModel()) + "\"";
  j += ",\"revision\":" + String(ESP.getChipRevision());
  j += ",\"cores\":" + String(ESP.getChipCores());
  j += ",\"mhz\":" + String(ESP.getCpuFreqMHz());
  j += ",\"flash\":" + String(ESP.getFlashChipSize());
  j += ",\"ram\":" + String(ESP.getHeapSize());
  j += ",\"psramTotal\":" + String(ESP.getPsramSize());
  j += ",\"mac\":\"" + WiFi.softAPmacAddress() + "\"";
  j += ",\"sdk\":\"" + String(ESP.getSdkVersion()) + "\"";
  j += ",\"wifi\":\"" + jsonText(WIFI_NAME) + "\"";
  j += ",\"channel\":" + String(wifiChannel);
  j += ",\"fractalW\":" + String(fractalW);
  j += ",\"fractalH\":" + String(fractalH);
  j += ",\"storageTotal\":" + String((uint32_t)storageTotal());
  j += ",\"storageFree\":" + String((uint32_t)storageFree());
  j += ",\"resetReason\":\"" + resetReason() + "\"";
  j += ",\"touchPins\":[";
  for (int p = 0; p < TOUCH_COUNT; p++) {
    if (p > 0) j += ",";
    j += String(TOUCH_PINS[p]);
  }
  j += "]}";
  server.send(200, "application/json", j);
}

// /led?color=ff8800&brightness=128&effect=1  (everything is optional)
void handleLed() {
  requestCounter++;
  if (server.hasArg("color")) {
    long color = strtol(server.arg("color").c_str(), nullptr, 16);   // hex -> number
    ledR = (color >> 16) & 0xFF;
    ledG = (color >> 8) & 0xFF;
    ledB = color & 0xFF;
  }
  if (server.hasArg("brightness")) {
    ledBrightness = constrain(server.arg("brightness").toInt(), 0, 255);
  }
  if (server.hasArg("effect")) {
    ledEffect = constrain(server.arg("effect").toInt(), 0, 7);
  }
  server.send(200, "text/plain", "ok");
}

void handleGameStart() {
  requestCounter++;
  gameStart();
  server.send(200, "text/plain", "ok");
}

// The touch task does the calibrating itself (so two tasks never measure at the same time). We wait until it is done.
void handleCalibrate() {
  requestCounter++;
  calibrateRequest = true;
  unsigned long start = millis();
  while (calibrateRequest && millis() - start < 3000) delay(10);
  server.send(calibrateRequest ? 503 : 200, "text/plain", calibrateRequest ? "busy" : "ok");
}

// /fractal?x=-0.6&y=0&s=3.2&i=100&c=2
void handleFractal() {
  requestCounter++;
  float width = server.arg("s").toFloat();      // how wide the image is
  if (width <= 0 || width > 10) width = 3.2;
  fCx = server.arg("x").toFloat();
  fCy = server.arg("y").toFloat();
  fStep = width / fractalW;
  fMaxIter = constrain(server.arg("i").toInt(), 10, 255);
  bool twoCores = server.arg("c") != "1";

  nextRow = 0;
  fractalSteps = 0;
  unsigned long start = micros();
  if (twoCores) {
    webTaskHandle = xTaskGetCurrentTaskHandle();
    xTaskNotifyGive(fractalTaskHandle);         // core 0: help with the computing!
    computeRows();                              // core 1 (here) computes too
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    // wait until core 0 is done as well
  } else {
    computeRows();                              // everything on core 1
  }
  unsigned long timeMs = (micros() - start) / 1000;

  server.sendHeader("X-Time", String(timeMs));
  server.sendHeader("X-Iter", String((uint32_t)fractalSteps));
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "application/octet-stream", (const char*)fractalPixels, fractalW * fractalH);
}

// WiFi scanner: start the first time, then keep asking until it is done
void handleScan() {
  requestCounter++;
  int n = WiFi.scanComplete();
  if (bleScanning()) {                          // Bluetooth is searching now (same antenna): wait a moment
    server.send(200, "application/json", "{\"busy\":true}");
    return;
  }
  if (n == -2) {                                // -2 = no scan running yet
    WiFi.scanNetworks(true, false, false, 120); // start in the background
    server.send(200, "application/json", "{\"busy\":true}");
    return;
  }
  if (n == -1) {                                // -1 = still busy
    server.send(200, "application/json", "{\"busy\":true}");
    return;
  }

  String j = "{\"busy\":false,\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) j += ",";
    j += "{\"name\":\"" + jsonText(WiFi.SSID(i)) + "\"";
    j += ",\"rssi\":" + String(WiFi.RSSI(i));
    j += ",\"channel\":" + String(WiFi.channel(i));
    j += ",\"open\":";
    j += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "true" : "false";
    j += "}";
  }
  j += "]}";
  WiFi.scanDelete();                            // clean up: search again next time
  server.send(200, "application/json", j);
}

void handleRestart() {
  requestCounter++;
  server.send(200, "text/plain", "ok");
  delay(300);
  ESP.restart();
}

void handleEmpty() {
  server.send(204, "text/plain", "");           // e.g. for favicon.ico
}

// Every unknown address -> send it to the main page (captive portal)
void handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}


// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP32-S3 Lab ===");

  // Get the reaction game record from the flash memory
  prefs.begin("lab", false);
  recordMs = prefs.getUInt("record", 0);

  // BOOT button with interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, FALLING);

  // Memory for the fractal: in PSRAM if there is any
  if (psramFound()) {
    fractalPixels = (uint8_t*)ps_malloc(fractalW * fractalH);
  }
  if (fractalPixels == nullptr) {               // no PSRAM: smaller image
    fractalW = 200;
    fractalH = 150;
    fractalPixels = (uint8_t*)malloc(fractalW * fractalH);
  }

  flashQueue = xQueueCreate(16, sizeof(BeatFlash));   // queue for the beat flashes

  // WiFi: our own network (AP) plus a station part for the WiFi scanner.
  // First check which channel (1, 6 or 11) is the quietest, so the neighbors bother you as little as possible.
  WiFi.mode(WIFI_AP_STA);
  wifiChannel = quietestChannel();
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD, wifiChannel, 0, 8);   // 0 = visible, max 8 devices
  delay(100);
  IPAddress ip = WiFi.softAPIP();
  dnsServer.start(53, "*", ip);                 // captive portal: every address -> ESP32

  touchCalibrate();                             // do not touch while starting up

  // Link web addresses to functions
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/info", handleInfo);
  server.on("/led", handleLed);
  server.on("/game/start", handleGameStart);
  server.on("/touch/calibrate", handleCalibrate);
  server.on("/fractal", handleFractal);
  server.on("/scan", handleScan);
  server.on("/ble", handleBle);
  server.on("/benchmark", handleBenchmark);
  server.on("/tasks", handleTasks);
  server.on("/beat/list", handleBeatList);
  server.on("/beat/save", HTTP_POST, handleBeatSave);
  server.on("/beat/load", handleBeatLoad);
  server.on("/restart", handleRestart);
  server.on("/favicon.ico", handleEmpty);
  server.onNotFound(handleNotFound);
  server.begin();
  server.enableNoDelay();                       // send small packets right away

  // Extra tasks. Core 0: the LED effects and the fractal helper. Core 1: touch and the live connection.
  xTaskCreatePinnedToCore(ledTask, "led", 4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(fractalTask, "fractal", 4096, nullptr, 1, &fractalTaskHandle, 0);
  xTaskCreatePinnedToCore(touchTask, "touch", 4096, nullptr, 3, nullptr, 1);
  liveStart();
  systemStart();

  Serial.print("WiFi network: ");
  Serial.print(WIFI_NAME);
  Serial.print(" (channel ");
  Serial.print(wifiChannel);
  Serial.println(")");
  Serial.print("Open in your browser: http://");
  Serial.println(ip);
  Serial.print("PSRAM: ");
  Serial.println(psramFound() ? "found" : "not found");
  Serial.println("Type 'help' in the Serial Monitor for the console.");
  storageStart();                               // the 9 MB disk for beats (in the background)
}


// =====================================================================
//  LOOP  -  runs on core 1
// =====================================================================
void loop() {
  server.handleClientFast();                    // handle requests from your phone
  // (the DNS server and the touch task run by themselves in the background)

  unsigned long now = millis();
  if (now - lastSecond >= 1000) {               // every second
    lastSecond = now;
    requestsPerSecond = requestCounter;
    requestCounter = 0;
    float t = temperatureRead();
    if (!isnan(t)) temperature = t;
    systemUpdate();                             // load per core and connected devices
  }

  gameUpdate();
  consoleUpdate();                              // commands from the Serial Monitor
  delay(1);                                     // short pause: this way other tasks get time too
}
