// =====================================================================
//  BLUETOOTH  -  searching for Bluetooth Low Energy devices nearby
// =====================================================================
//  Many devices keep calling out "here I am!" for a short moment via
//  Bluetooth Low Energy (BLE): phones, earbuds, watches, laptops. The ESP32
//  listens to that for 5 seconds. WiFi and Bluetooth share the same antenna:
//  the chip switches between the two super fast (that is called "coexistence").
// =====================================================================
#include <BLEDevice.h>
#include <BLEScan.h>

bool bleStarted = false;
volatile bool bleBusy = false;                  // a search is running right now
volatile bool bleReady = false;                 // there are results waiting
BLEScan* bleScan = nullptr;

// Start a 5-second search in the background
bool bleStart() {
  if (!bleStarted) {
    BLEDevice::init("ESP32-Lab");               // only switch Bluetooth on when you use it for the first time
    bleScan = BLEDevice::getScan();
    bleScan->setActiveScan(true);               // active: also ask devices for their name
    bleScan->setInterval(100);                  // every 100 ms...
    bleScan->setWindow(50);                     // ...listen for 50 ms: this leaves time for WiFi
    bleStarted = true;
  }
  bleScan->clearResults();
  bleReady = false;
  // The function between [] is called when the 5 seconds are over
  bleBusy = bleScan->start(5, [](BLEScanResults results) { bleBusy = false; bleReady = true; }, false);
  return bleBusy;
}

// Who made the device? That is in the first 2 bytes of the "manufacturer data".
String bleMaker(const String& data) {
  if (data.length() < 2) return "";
  uint16_t id = (uint8_t)data[0] | ((uint8_t)data[1] << 8);
  switch (id) {
    case 0x004C: return "Apple";
    case 0x0006: return "Microsoft";
    case 0x0075: return "Samsung";
    case 0x00E0: return "Google";
    case 0x0087: return "Garmin";
    case 0x038F: return "Xiaomi";
    case 0x027D: return "Huawei";
    case 0x0157: return "Huami (Amazfit)";
    case 0x009E: return "Bose";
    case 0x012D: return "Sony";
    case 0x0171: return "Amazon";
    case 0x0059: return "Nordic";
    case 0x02E5: return "Espressif (an ESP32!)";
    default:     return "";
  }
}

int bleOrder[256], bleStrength[256];            // to sort the list by signal strength

// The results as JSON (closest first, 60 at most), and then clean up
String bleResultsJson() {
  BLEScanResults* r = bleScan->getResults();
  String j = "{\"busy\":false,\"devices\":[";
  int n = r ? min(r->getCount(), 256) : 0;
  for (int i = 0; i < n; i++) {
    bleOrder[i] = i;
    bleStrength[i] = r->getDevice(i).getRSSI();
  }
  // Sorting: strongest signal at the top ("insertion sort": slide each device to its place)
  for (int i = 1; i < n; i++) {
    int index = bleOrder[i], strength = bleStrength[i], k = i - 1;
    while (k >= 0 && bleStrength[k] < strength) {
      bleOrder[k + 1] = bleOrder[k];
      bleStrength[k + 1] = bleStrength[k];
      k--;
    }
    bleOrder[k + 1] = index;
    bleStrength[k + 1] = strength;
  }
  for (int i = 0; i < n && i < 60; i++) {
    BLEAdvertisedDevice a = r->getDevice(bleOrder[i]);
    if (i > 0) j += ",";
    j += "{\"name\":\"" + jsonText(a.haveName() ? a.getName() : String("")) + "\"";
    j += ",\"rssi\":" + String(a.getRSSI());
    j += ",\"maker\":\"" + jsonText(a.haveManufacturerData() ? bleMaker(a.getManufacturerData()) : String("")) + "\"";
    j += "}";
  }
  j += "]}";
  bleScan->clearResults();
  bleReady = false;
  return j;
}

bool bleScanning() {
  return bleBusy;
}

// /ble: start the first time, then keep asking until it is done (just like the WiFi scanner)
void handleBle() {
  requestCounter++;
  if (bleBusy || WiFi.scanComplete() == -1) {   // is a WiFi scan running? then we wait a moment (same antenna)
    server.send(200, "application/json", "{\"busy\":true}");
  } else if (bleReady) {
    server.send(200, "application/json", bleResultsJson());
  } else if (bleStart()) {
    server.send(200, "application/json", "{\"busy\":true}");
  } else {
    server.send(200, "application/json", "{\"error\":\"Bluetooth could not start.\"}");
  }
}

// Command "ble" in the serial console
void bleConsoleScan() {
  if (bleBusy) { Serial.println("A Bluetooth search is already running."); return; }
  Serial.println("Listening (5 seconds)...");
  if (!bleStart()) { Serial.println("Bluetooth could not start."); return; }
  unsigned long start = millis();
  while (bleBusy && millis() - start < 8000) delay(20);
  BLEScanResults* r = bleScan->getResults();
  int n = r ? r->getCount() : 0;
  for (int i = 0; i < n; i++) {
    BLEAdvertisedDevice a = r->getDevice(i);
    String maker = a.haveManufacturerData() ? bleMaker(a.getManufacturerData()) : String("");
    Serial.printf("%4d dBm  %-20s %s\n", a.getRSSI(), a.haveName() ? a.getName().c_str() : "(no name)", maker.c_str());
  }
  Serial.printf("%d devices found.\n", n);
  bleScan->clearResults();
  bleReady = false;
}
