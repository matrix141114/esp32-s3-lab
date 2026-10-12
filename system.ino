// =====================================================================
//  SYSTEM  -  load per core, task list, connected devices
//             and the serial console
// =====================================================================
//  FreeRTOS (the "mini operating system" of the ESP32) keeps track, per task,
//  of how many microseconds that task has been running. Each core also has an
//  "idle" task: it only runs when the core has nothing else to do.
//  So: the less time the idle task got, the busier the core was.
// =====================================================================
#include <esp_wifi.h>
#include <esp_system.h>

const int MAX_TASKS = 32;
TaskStatus_t tasksNow[MAX_TASKS];               // the latest measurement of all tasks
TaskHandle_t prevHandle[MAX_TASKS];             // the measurement from one second earlier...
uint32_t prevRunTime[MAX_TASKS];                // ...to be able to calculate the difference
int prevCount = 0;
uint32_t prevTotal = 0;

// What we show on the Info page (refreshed every second)
char taskName[MAX_TASKS][configMAX_TASK_NAME_LEN];
int taskCore[MAX_TASKS], taskPrio[MAX_TASKS];
uint32_t taskStack[MAX_TASKS];
float taskCpu[MAX_TASKS];
int taskCount = 0;

void systemStart() {
  prevTotal = 0;
  prevCount = 0;
}

// Called every second from loop()
void systemUpdate() {
  // 1. Load per core and per task
  uint32_t total = 0;
  int n = uxTaskGetSystemState(tasksNow, MAX_TASKS, &total);
  uint32_t elapsed = total - prevTotal;         // microseconds since the previous measurement
  if (n > 0 && prevTotal != 0 && elapsed > 0) {
    TaskHandle_t idle0 = xTaskGetIdleTaskHandleForCore(0);
    TaskHandle_t idle1 = xTaskGetIdleTaskHandleForCore(1);
    taskCount = 0;
    for (int i = 0; i < n; i++) {
      // how long did this task run during the last second?
      uint32_t before = tasksNow[i].ulRunTimeCounter;   // new task: everything counts
      for (int k = 0; k < prevCount; k++) {
        if (prevHandle[k] == tasksNow[i].xHandle) { before = prevRunTime[k]; break; }
      }
      float percent = (tasksNow[i].ulRunTimeCounter - before) * 100.0f / elapsed;
      if (percent > 100) percent = 100;
      if (tasksNow[i].xHandle == idle0) cpuLoad[0] = 100 - percent;
      if (tasksNow[i].xHandle == idle1) cpuLoad[1] = 100 - percent;
      strncpy(taskName[taskCount], tasksNow[i].pcTaskName, configMAX_TASK_NAME_LEN - 1);
      taskName[taskCount][configMAX_TASK_NAME_LEN - 1] = 0;
      BaseType_t core = tasksNow[i].xCoreID;
      taskCore[taskCount] = (core == 0 || core == 1) ? core : -1;     // -1 = may run on both
      taskPrio[taskCount] = tasksNow[i].uxCurrentPriority;
      taskStack[taskCount] = tasksNow[i].usStackHighWaterMark;        // in bytes: this much stack was never used
      taskCpu[taskCount] = percent;
      taskCount++;
    }
  }
  for (int i = 0; i < n; i++) {                 // remember for the next time
    prevHandle[i] = tasksNow[i].xHandle;
    prevRunTime[i] = tasksNow[i].ulRunTimeCounter;
  }
  prevCount = n;
  prevTotal = total;

  // 2. Which devices are connected to our WiFi network, and how strong is their signal?
  wifi_sta_list_t list;
  if (esp_wifi_ap_get_sta_list(&list) == ESP_OK) {
    int count = min((int)list.num, 10);
    for (int i = 0; i < count; i++) deviceRssi[i] = list.sta[i].rssi;
    rssiCount = count;
    deviceCount = list.num;
  }
}

// /tasks  ->  {"cpu":[12.3,4.5],"tasks":[{"name":"loopTask","core":1,"prio":1,"stack":5000,"cpu":3.2},...]}
void handleTasks() {
  requestCounter++;
  String j = "{\"cpu\":[" + String(cpuLoad[0], 1) + "," + String(cpuLoad[1], 1) + "],\"tasks\":[";
  for (int i = 0; i < taskCount; i++) {
    if (i > 0) j += ",";
    j += "{\"name\":\"" + jsonText(taskName[i]) + "\"";
    j += ",\"core\":" + String(taskCore[i]);
    j += ",\"prio\":" + String(taskPrio[i]);
    j += ",\"stack\":" + String(taskStack[i]);
    j += ",\"cpu\":" + String(taskCpu[i], 1) + "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

// Which WiFi channel (1, 6 or 11) is the quietest? Channels that are close to each other
// also disturb each other: a network on channel 3 therefore counts a little for channel 1 and 6.
// A strong network (close by) counts more than a weak network (far away).
int quietestChannel() {
  const int CHANNELS[3] = {1, 6, 11};
  float crowding[3] = {0, 0, 0};
  int n = WiFi.scanNetworks(false, true);       // a quick search (also hidden networks)
  for (int i = 0; i < n; i++) {
    float strength = constrain(WiFi.RSSI(i) + 100, 1, 70);
    for (int c = 0; c < 3; c++) {
      int distance = abs(WiFi.channel(i) - CHANNELS[c]);
      if (distance < 5) crowding[c] += strength * (5 - distance) / 5.0f;
    }
  }
  WiFi.scanDelete();
  int best = 0;
  for (int c = 1; c < 3; c++) {
    if (crowding[c] < crowding[best]) best = c;
  }
  Serial.printf("%d networks nearby. Crowding on channel 1: %.0f, 6: %.0f, 11: %.0f\n",
                max(n, 0), crowding[0], crowding[1], crowding[2]);
  return CHANNELS[best];
}

// Why did the ESP32 start up again the last time?
String resetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "power switched on";
    case ESP_RST_EXT:       return "reset button";
    case ESP_RST_SW:        return "restart by the program";
    case ESP_RST_PANIC:     return "crash (error in the program)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "watchdog (something got stuck)";
    case ESP_RST_DEEPSLEEP: return "woke up from deep sleep";
    case ESP_RST_BROWNOUT:  return "voltage too low";
    case ESP_RST_USB:       return "via USB";
    default:                return "unknown";
  }
}


// =====================================================================
//  SERIAL CONSOLE  -  type a command in the Serial Monitor (115200 baud)
// =====================================================================
void consoleHelp() {
  Serial.println("Commands:");
  Serial.println("  status     temperature, memory, load, connections");
  Serial.println("  tasks      all FreeRTOS tasks with their core and load");
  Serial.println("  touch      the readings of the 14 touch pins");
  Serial.println("  benchmark  all speed tests (takes a few seconds)");
  Serial.println("  wifi       search for WiFi networks nearby");
  Serial.println("  ble        search for Bluetooth devices nearby");
  Serial.println("  restart    start the ESP32 up again");
}

void consoleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  Serial.println("> " + cmd);
  if (cmd == "help" || cmd == "?") {
    consoleHelp();
  } else if (cmd == "status") {
    Serial.printf("Temperature: %.1f C   up for: %lu s\n", temperature, millis() / 1000);
    Serial.printf("Free RAM: %u bytes (lowest ever: %u)   free PSRAM: %u bytes\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getFreePsram());
    Serial.printf("Core 0: %.1f%% busy   core 1: %.1f%% busy\n", cpuLoad[0], cpuLoad[1]);
    Serial.printf("Devices on WiFi: %d   live connections: %d   web requests/s: %lu\n",
                  deviceCount, liveConnected(), requestsPerSecond);
    Serial.printf("Storage: %u kB free of %u kB   last restart: %s\n",
                  (unsigned)(storageFree() / 1024), (unsigned)(storageTotal() / 1024), resetReason().c_str());
  } else if (cmd == "tasks") {
    Serial.println("Task              core  prio  busy    stack free");
    for (int i = 0; i < taskCount; i++) {
      Serial.printf("%-16s  %4s  %4d  %5.1f%%  %6u B\n", taskName[i],
                    taskCore[i] < 0 ? "both" : (taskCore[i] == 0 ? "0" : "1"), taskPrio[i], taskCpu[i], (unsigned)taskStack[i]);
    }
  } else if (cmd == "touch") {
    for (int p = 0; p < TOUCH_COUNT; p++) {
      Serial.printf("GPIO%-2d  reading %7u  baseline %9.1f  diff %3d%%  %s\n", TOUCH_PINS[p],
                    (unsigned)touchValue[p], touchBaseline[p], touchPercent(p), touchActive[p] ? "TOUCHED" : "");
    }
  } else if (cmd == "benchmark") {
    for (int t = 0; t < benchmarkCount(); t++) {
      float w = benchmarkRun(t);
      Serial.printf("%-37s %10.2f %s\n", benchmarkName(t), w, benchmarkUnit(t));
    }
  } else if (cmd == "wifi") {
    Serial.println("Searching...");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      Serial.printf("%4d dBm  channel %2d  %s\n", WiFi.RSSI(i), WiFi.channel(i), WiFi.SSID(i).c_str());
    }
    if (n <= 0) Serial.println("Nothing found (or a scan from the web page was already running).");
    WiFi.scanDelete();
  } else if (cmd == "ble") {
    bleConsoleScan();
  } else if (cmd == "restart") {
    Serial.println("See you soon!");
    delay(100);
    ESP.restart();
  } else {
    Serial.println("Unknown command. Type 'help'.");
  }
}

void consoleUpdate() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (line.length() > 0) consoleCommand(line);
      line = "";
    } else if (line.length() < 40) {
      line += c;
    }
  }
}
