// =====================================================================
//  STORAGE  -  saving beats in the flash memory (FAT file system)
// =====================================================================
//  Of the 16 MB of flash, 9 MB is a separate "disk" with a FAT file
//  system, just like a USB stick. That is where we keep your beats as small
//  text files (JSON), for example /beats/deep-1.json.
//  They stay saved when the power goes off.
// =====================================================================
#include <FFat.h>

volatile bool storageReady = false;             // only true once the disk is ready

// The very first time the disk has to be formatted, and that takes a while.
// That is why we do it in a separate task: the rest of the program just starts up.
void storageTask(void* parameter) {
  if (FFat.begin(true)) {                       // true = format if it does not work yet
    if (!FFat.exists("/beats")) FFat.mkdir("/beats");
    storageReady = true;
    Serial.printf("Storage: ready, %u kB free of %u kB\n",
                  (unsigned)(FFat.freeBytes() / 1024), (unsigned)(FFat.totalBytes() / 1024));
  } else {
    Serial.println("Storage: the FAT partition could not be mounted");
  }
  vTaskDelete(nullptr);                         // done: this task cleans itself up
}

void storageStart() {
  xTaskCreatePinnedToCore(storageTask, "storage", 6144, nullptr, 1, nullptr, 0);
}

size_t storageTotal() { return storageReady ? FFat.totalBytes() : 0; }
size_t storageFree()  { return storageReady ? FFat.freeBytes() : 0; }

// File name for ?kit=deep&slot=1  ->  /beats/deep-1.json  (or "" if it is not valid)
String beatPath() {
  String kit = server.arg("kit");
  int slot = server.arg("slot").toInt();
  if ((kit != "deep" && kit != "afro") || slot < 1 || slot > 4) return "";
  return "/beats/" + kit + "-" + String(slot) + ".json";
}

// POST /beat/save?kit=deep&slot=1   (the beat itself is in the body)
void handleBeatSave() {
  requestCounter++;
  String path = beatPath();
  String content = server.arg("plain");
  if (!storageReady) { server.send(503, "text/plain", "storage not ready yet"); return; }
  if (path == "" || content.length() < 2 || content.length() > 8000 || content[0] != '{') {
    server.send(400, "text/plain", "error");
    return;
  }
  // First write to a temporary file. If the power goes off halfway,
  // your old beat stays intact. Only when everything is in it does the file get its real name.
  String tempPath = path + ".tmp";
  File f = FFat.open(tempPath, FILE_WRITE);
  if (!f) { server.send(500, "text/plain", "cannot write"); return; }
  size_t written = f.print(content);
  f.close();
  if (written != content.length()) {
    FFat.remove(tempPath);
    server.send(500, "text/plain", "disk full?");
    return;
  }
  if (FFat.exists(path)) FFat.remove(path);
  if (FFat.rename(tempPath, path)) server.send(200, "text/plain", "ok");
  else server.send(500, "text/plain", "rename failed");
}

// GET /beat/load?kit=deep&slot=1
void handleBeatLoad() {
  requestCounter++;
  String path = beatPath();
  if (!storageReady || path == "" || !FFat.exists(path)) {
    server.send(404, "application/json", "{\"error\":\"empty\"}");
    return;
  }
  File f = FFat.open(path, FILE_READ);
  if (!f) { server.send(500, "application/json", "{\"error\":\"cannot read\"}"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "application/json");
  f.close();
}

// GET /beat/list  ->  which slots are filled?  {"deep":[true,false,false,false],"afro":[...]}
void handleBeatList() {
  requestCounter++;
  const char* kits[] = {"deep", "afro"};
  String j = "{";
  for (int k = 0; k < 2; k++) {
    if (k > 0) j += ",";
    j += "\"" + String(kits[k]) + "\":[";
    for (int n = 1; n <= 4; n++) {
      if (n > 1) j += ",";
      bool exists = storageReady && FFat.exists("/beats/" + String(kits[k]) + "-" + String(n) + ".json");
      j += exists ? "true" : "false";
    }
    j += "]";
  }
  j += "}";
  server.send(200, "application/json", j);
}
