// =====================================================================
//  LIVE CONNECTION  -  a WebSocket on port 81
// =====================================================================
//  Without a WebSocket your phone keeps asking: "is there anything new?"
//  Every question is a new connection, and that takes time.
//  With a WebSocket the connection stays open. The ESP32 then sends a
//  message by itself, right away, when you touch a pin or press BOOT.
//  That saves tens of milliseconds: nice for the drums and the synth.
//
//  This is how it works:
//  1. The browser asks with normal HTTP: "may this become a WebSocket?"
//     and sends a random key along.
//  2. The ESP32 calculates an answer to it with SHA-1 and says "yes".
//  3. After that they send small packets ("frames") back and forth.
//
//  This code runs in its own task (liveTask), so the web server in
//  loop() does not have to wait for it.
// =====================================================================
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>
#include <lwip/sockets.h>

const int LIVE_PORT = 81;
const int LIVE_MAX = 4;                         // this many screens at the same time
const int LIVE_BUF = 256;                       // receive buffer per screen (messages are small)

WiFiServer liveServer(LIVE_PORT);
WiFiClient liveClient[LIVE_MAX];
uint8_t liveIn[LIVE_MAX][LIVE_BUF];             // what comes in from each screen
int liveInLength[LIVE_MAX];
unsigned long liveLastHeard[LIVE_MAX];          // millis() of the latest message from that screen
TaskHandle_t liveTaskHandle = nullptr;
volatile int liveCount = 0;                     // how many screens are live connected now

// Signal to the live task: "something changed, send the status right away!"
void liveNotify() {
  if (liveTaskHandle) xTaskNotifyGive(liveTaskHandle);
}

int liveConnected() {
  return liveCount;
}

void liveClose(int k) {
  liveClient[k].stop();
  liveInLength[k] = 0;
}

// Send a frame to screen k. opcode: 1 = text, 8 = close, 10 = pong.
// We never wait: if the send buffer is full for a moment (phone too slow), we skip this message.
bool liveSend(int k, const char* data, size_t n, uint8_t opcode) {
  static uint8_t packet[4 + 2048];              // only the live task uses this
  if (n > 2048) return false;
  size_t headerLen;
  packet[0] = 0x80 | opcode;                    // 0x80 = this is the whole message (FIN)
  if (n < 126) {
    packet[1] = n;
    headerLen = 2;
  } else {
    packet[1] = 126;                            // the length is in the next 2 bytes
    packet[2] = n >> 8;
    packet[3] = n & 0xFF;
    headerLen = 4;
  }
  memcpy(packet + headerLen, data, n);
  int fd = liveClient[k].fd();
  if (fd < 0) return false;
  // Is there enough room in the send buffer? If not (phone a bit slow), we skip this
  // message. Sending half of it is not allowed: then the WebSocket gets confused.
  fd_set writeSet;
  FD_ZERO(&writeSet);
  FD_SET(fd, &writeSet);
  struct timeval zero = {0, 0};
  if (select(fd + 1, nullptr, &writeSet, nullptr, &zero) <= 0) return true;
  int r = send(fd, packet, headerLen + n, MSG_DONTWAIT);
  if (r == (int)(headerLen + n)) return true;
  if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;   // full after all: skip
  liveClose(k);                                 // half sent or an error: the screen connects again
  return false;
}

// Step 1 and 2: read the HTTP question and say "yes"
bool liveHandshake(WiFiClient& c) {
  String key, line;
  bool upgrade = false, done = false;
  unsigned long start = millis();
  while (!done && millis() - start < 1500) {
    if (!c.available()) {
      if (!c.connected()) return false;
      delay(2);
      continue;
    }
    char ch = c.read();
    if (ch == '\r') continue;
    if (ch != '\n') {
      if (line.length() < 200) line += ch;
      continue;
    }
    if (line.length() == 0) {                   // empty line = end of the question
      done = true;
      continue;
    }
    String lower = line;
    lower.toLowerCase();
    if (lower.startsWith("sec-websocket-key:")) {
      key = line.substring(18);
      key.trim();
    }
    if (lower.startsWith("upgrade:") && lower.indexOf("websocket") > 0) upgrade = true;
    line = "";
  }
  if (!done || !upgrade || key.length() == 0 || key.length() > 60) {
    c.print("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    return false;
  }
  // answer = base64( SHA-1( key + a fixed text from the WebSocket standard ) )
  String text = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  uint8_t hash[20];
  mbedtls_sha1((const uint8_t*)text.c_str(), text.length(), hash);
  uint8_t answer[32];
  size_t length = 0;
  mbedtls_base64_encode(answer, sizeof(answer), &length, hash, sizeof(hash));
  c.print("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
  c.write(answer, length);
  c.print("\r\n\r\n");
  return true;
}

// A text message from a screen
void liveMessage(int k, const char* text, size_t n) {
  if (text[0] == 'p') {                         // little ping: send it straight back (this is how the page measures the round trip)
    liveSend(k, text, n, 0x1);
  } else if (text[0] == 'f') {                  // LED flash on the beat: f<in how many ms>,<hue>,<strength>
    int ms = 0, hue = 0, strength = 255;
    if (sscanf(text + 1, "%d,%d,%d", &ms, &hue, &strength) >= 2) ledFlash(ms, hue, strength);
  }
}

// Read what screen k has sent and take whole frames out of it
void liveRead(int k) {
  WiFiClient& c = liveClient[k];
  int available = c.available();
  int space = LIVE_BUF - liveInLength[k];
  if (available > 0 && space > 0) {
    int n = c.read(liveIn[k] + liveInLength[k], min(available, space));
    if (n > 0) liveInLength[k] += n;
  }
  while (liveInLength[k] >= 2) {
    uint8_t* b = liveIn[k];
    uint8_t opcode = b[0] & 0x0F;
    bool fin = b[0] & 0x80;                     // not a message in pieces
    bool masked = b[1] & 0x80;                  // a browser must always mask its messages
    size_t length = b[1] & 0x7F, pos = 2;
    if (length == 126) {
      if (liveInLength[k] < 4) return;
      length = (b[2] << 8) | b[3];
      pos = 4;
    }
    if (length > 200 || !masked || !fin) {      // we do not expect messages like that
      liveClose(k);
      return;
    }
    int total = pos + 4 + length;
    if (liveInLength[k] < total) return;        // the rest is still coming
    const uint8_t* mask = b + pos;
    char text[201];
    for (size_t i = 0; i < length; i++) text[i] = b[pos + 4 + i] ^ mask[i % 4];
    text[length] = 0;
    memmove(b, b + total, liveInLength[k] - total);
    liveInLength[k] -= total;
    liveLastHeard[k] = millis();
    if (opcode == 0x8) {                        // the screen wants to stop
      liveSend(k, "", 0, 0x8);
      liveClose(k);
      return;
    }
    if (opcode == 0x9) liveSend(k, text, length, 0xA);   // ping -> pong
    else if (opcode == 0x1 && length > 0) liveMessage(k, text, length);
  }
}

void liveTask(void* parameter) {
  liveServer.begin();
  liveServer.setNoDelay(true);
  unsigned long lastStatus = 0;
  while (true) {
    // sleep until a signal arrives (touch, button), but 10 ms at most
    bool notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10)) > 0;

    // 1. A new screen that wants to connect?
    WiFiClient newClient = liveServer.accept();
    if (newClient) {
      int freeSlot = -1;
      for (int k = 0; k < LIVE_MAX; k++) {
        if (!liveClient[k].connected()) { freeSlot = k; break; }
      }
      if (freeSlot >= 0 && liveHandshake(newClient)) {
        newClient.setNoDelay(true);             // send small packets right away
        liveClient[freeSlot] = newClient;
        liveInLength[freeSlot] = 0;
        liveLastHeard[freeSlot] = millis();
        notified = true;                        // send the first status right away
      } else {
        newClient.stop();
      }
    }

    // 2. Read messages, and clean up screens that said nothing for 5 seconds
    int count = 0;
    for (int k = 0; k < LIVE_MAX; k++) {
      if (!liveClient[k].connected()) continue;
      if (millis() - liveLastHeard[k] > 5000) { liveClose(k); continue; }
      liveRead(k);
      if (liveClient[k].connected()) count++;
    }
    liveCount = count;

    // 3. Send the status: 25 times per second, or right after a signal
    if (count > 0 && (notified || millis() - lastStatus >= 40)) {
      lastStatus = millis();
      String j = makeStatusJson();
      for (int k = 0; k < LIVE_MAX; k++) {
        if (liveClient[k].connected()) liveSend(k, j.c_str(), j.length(), 0x1);
      }
    }
  }
}

void liveStart() {
  xTaskCreatePinnedToCore(liveTask, "live", 6144, nullptr, 2, &liveTaskHandle, 1);
}
