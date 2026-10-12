// =====================================================================
//  BENCHMARK  -  how fast is the ESP32-S3 really?
// =====================================================================
//  Each test does as much of the same work as possible for 0.3 seconds,
//  and counts how often that worked. This way you can compare things fairly:
//   - float (with the FPU math helper) against double (without FPU)
//   - 1 core against 2 cores
//   - plain C code against vector instructions (SIMD)
//   - the internal RAM against the PSRAM and the flash memory
//   - the special hardware for SHA-256, AES and random numbers
// =====================================================================
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include "dsps_dotprod.h"                       // esp-dsp: math functions with vector instructions

const int BENCH_COUNT = 13;
const int64_t BENCH_DURATION = 300000;          // each test 0.3 seconds (in microseconds)
const char* BENCH_ID[BENCH_COUNT] = {
  "int", "float", "double", "cores1", "cores2", "plain", "simd", "sram", "psram", "flash", "sha", "aes", "trng" };
const char* BENCH_NAME[BENCH_COUNT] = {
  "Whole numbers (int)", "Decimal numbers (float) with the FPU", "Extra precise (double), without FPU",
  "Float on 1 core", "Float on 2 cores at once",
  "Vector sum with plain C code", "Vector sum with vector instructions",
  "Copying in the internal RAM", "Copying in the PSRAM", "Reading from the flash memory",
  "SHA-256 in hardware", "AES-128 encryption in hardware", "True random numbers" };
const char* BENCH_UNIT[BENCH_COUNT] = {
  "million/s", "million/s", "million/s", "million/s", "million/s", "million/s", "million/s",
  "MB/s", "MB/s", "MB/s", "MB/s", "MB/s", "kB/s" };
const char* BENCH_GROUP[BENCH_COUNT] = {
  "compute", "compute", "compute", "cores", "cores", "simd", "simd",
  "memory", "memory", "memory", "hardware", "hardware", "hardware" };

// We store the results in here. Otherwise the compiler sees that nobody uses the
// result, and it cleverly skips all the computing.
volatile uint32_t benchSinkI;
volatile float benchSinkF;
volatile double benchSinkD;

int benchmarkCount() { return BENCH_COUNT; }
const char* benchmarkName(int t) { return BENCH_NAME[t]; }
const char* benchmarkUnit(int t) { return BENCH_UNIT[t]; }

// 'count' things in 'us' microseconds: how many million per second is that?
// (per microsecond = million per second, and bytes per microsecond = MB per second)
float perSecond(double count, int64_t us) {
  return us > 0 ? count / us : 0;
}

// We let the compiler make all compute tests below extra fast (O2), just like the fractal.
#pragma GCC push_options
#pragma GCC optimize("O2")

// --- Computing: again and again x = x * m + c, in 4 rows side by side ---
float testInt() {
  uint32_t a = esp_random() | 1, s0 = 1, s1 = 2, s2 = 3, s3 = 4, rounds = 0;
  int64_t start = esp_timer_get_time(), now;
  do {
    for (uint32_t i = 0; i < 1000; i++) {
      s0 = s0 * a + i; s1 = s1 * a + i; s2 = s2 * a + i; s3 = s3 * a + i;
    }
    rounds++;
    now = esp_timer_get_time();
  } while (now - start < BENCH_DURATION);
  benchSinkI = s0 + s1 + s2 + s3;
  return perSecond(rounds * 4000.0, now - start);
}

// This many rounds of float computing (of 4000 steps) fit in 'duration' microseconds
uint32_t floatRounds(int64_t duration) {
  float x0 = 1.0f + (esp_random() & 7), x1 = x0 + 1, x2 = x0 + 2, x3 = x0 + 3;
  const float m = 0.999f, c = 0.001f;
  uint32_t rounds = 0;
  int64_t start = esp_timer_get_time();
  do {
    for (int i = 0; i < 1000; i++) {
      x0 = x0 * m + c; x1 = x1 * m + c; x2 = x2 * m + c; x3 = x3 * m + c;
    }
    rounds++;
  } while (esp_timer_get_time() - start < duration);
  benchSinkF = x0 + x1 + x2 + x3;
  return rounds;
}

float testFloat() {
  int64_t start = esp_timer_get_time();
  uint32_t rounds = floatRounds(BENCH_DURATION);
  return perSecond(rounds * 4000.0, esp_timer_get_time() - start);
}

float testDouble() {
  double x0 = 1.0 + (esp_random() & 7), x1 = x0 + 1, x2 = x0 + 2, x3 = x0 + 3;
  const double m = 0.999, c = 0.001;
  uint32_t rounds = 0;
  int64_t start = esp_timer_get_time(), now;
  do {
    for (int i = 0; i < 100; i++) {             // less per round: double is much slower
      x0 = x0 * m + c; x1 = x1 * m + c; x2 = x2 * m + c; x3 = x3 * m + c;
    }
    rounds++;
    now = esp_timer_get_time();
  } while (now - start < BENCH_DURATION);
  benchSinkD = x0 + x1 + x2 + x3;
  return perSecond(rounds * 400.0, now - start);
}

// --- Two cores: core 0 gets a helper task with exactly the same work ---
volatile uint32_t helperRounds = 0;
TaskHandle_t benchWaiter = nullptr;

void benchHelperTask(void* parameter) {
  helperRounds = floatRounds(BENCH_DURATION);
  xTaskNotifyGive(benchWaiter);                 // signal: done!
  vTaskDelete(nullptr);
}

float testTwoCores() {
  benchWaiter = xTaskGetCurrentTaskHandle();
  helperRounds = 0;
  int64_t start = esp_timer_get_time();
  xTaskCreatePinnedToCore(benchHelperTask, "bench", 4096, nullptr, 1, nullptr, 0);   // 0.3 s: well within the watchdog time
  uint32_t own = floatRounds(BENCH_DURATION);   // core 1 (here) computes at the same time
  ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
  return perSecond((own + helperRounds) * 4000.0, esp_timer_get_time() - start);
}

// --- Vector instructions: a "dot product" (multiply pairs and add everything up) ---
float testVector(bool simd) {
  const int N = 1024;
  float* a = (float*)heap_caps_aligned_alloc(16, N * sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  float* b = (float*)heap_caps_aligned_alloc(16, N * sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  float w = 0;
  if (a && b) {
    for (int i = 0; i < N; i++) { a[i] = i * 0.001f; b[i] = 1.0f - i * 0.0005f; }
    float result = 0;
    uint32_t rounds = 0;
    int64_t start = esp_timer_get_time(), now;
    do {
      if (simd) dsps_dotprod_f32_aes3(a, b, &result, N);   // with the vector instructions of the S3
      else dsps_dotprod_f32_ansi(a, b, &result, N);        // the same sum in plain C
      rounds++;
      now = esp_timer_get_time();
    } while (now - start < BENCH_DURATION);
    benchSinkF = result;
    w = perSecond((double)rounds * N, now - start);
  }
  heap_caps_free(a);
  heap_caps_free(b);
  return w;
}

#pragma GCC pop_options

// --- Memory: copying a block again and again ---------------------------
float testCopy(uint32_t caps, size_t size) {
  uint8_t* src = (uint8_t*)heap_caps_malloc(size, caps);
  uint8_t* dst = (uint8_t*)heap_caps_malloc(size, caps);
  float w = 0;
  if (src && dst) {
    memset(src, 0x5A, size);
    double bytes = 0;
    int64_t start = esp_timer_get_time(), now;
    do {
      memcpy(dst, src, size);
      bytes += size;
      now = esp_timer_get_time();
    } while (now - start < BENCH_DURATION);
    benchSinkI = dst[size / 2];
    w = perSecond(bytes, now - start);
  }
  heap_caps_free(src);
  heap_caps_free(dst);
  return w;
}

// Reading from the FAT partition of the flash memory (read only, nothing changes)
float testFlash() {
  const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, nullptr);
  const size_t BLOCK = 16384;
  uint8_t* buf = (uint8_t*)heap_caps_malloc(BLOCK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  float w = 0;
  if (part && buf) {
    size_t offset = 0;
    double bytes = 0;
    int64_t start = esp_timer_get_time(), now;
    do {
      esp_partition_read(part, offset, buf, BLOCK);
      offset = (offset + BLOCK) % (1024 * 1024);   // keep reading the first MB round and round
      bytes += BLOCK;
      now = esp_timer_get_time();
    } while (now - start < BENCH_DURATION);
    benchSinkI = buf[0];
    w = perSecond(bytes, now - start);
  }
  heap_caps_free(buf);
  return w;
}

// --- Special hardware --------------------------------------------------
float testSha() {
  const size_t N = 16384;
  uint8_t* buf = (uint8_t*)heap_caps_malloc(N, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  float w = 0;
  if (buf) {
    memset(buf, 0xA5, N);
    uint8_t digest[32];
    double bytes = 0;
    int64_t start = esp_timer_get_time(), now;
    do {
      mbedtls_sha256(buf, N, digest, 0);        // 0 = SHA-256 (not SHA-224)
      bytes += N;
      now = esp_timer_get_time();
    } while (now - start < BENCH_DURATION);
    benchSinkI = digest[0];
    w = perSecond(bytes, now - start);
  }
  heap_caps_free(buf);
  return w;
}

float testAes() {
  const size_t N = 16384;
  uint8_t* in = (uint8_t*)heap_caps_malloc(N, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  uint8_t* out = (uint8_t*)heap_caps_malloc(N, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  float w = 0;
  if (in && out) {
    memset(in, 0x3C, N);
    uint8_t key[16], iv[16] = {0};
    esp_fill_random(key, sizeof(key));
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    double bytes = 0;
    int64_t start = esp_timer_get_time(), now;
    do {
      mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, N, iv, in, out);
      bytes += N;
      now = esp_timer_get_time();
    } while (now - start < BENCH_DURATION);
    mbedtls_aes_free(&aes);
    benchSinkI = out[0];
    w = perSecond(bytes, now - start);
  }
  heap_caps_free(in);
  heap_caps_free(out);
  return w;
}

// True random numbers come from radio noise. The chip keeps waiting a moment until there is
// enough new noise: that is why this is much slower than computing (in kB/s, not MB/s).
float testRandom() {
  uint8_t buf[1024];
  double bytes = 0;
  int64_t start = esp_timer_get_time(), now;
  do {
    esp_fill_random(buf, sizeof(buf));
    bytes += sizeof(buf);
    now = esp_timer_get_time();
  } while (now - start < BENCH_DURATION);
  benchSinkI = buf[0];
  return perSecond(bytes, now - start) * 1000;  // MB/s -> kB/s
}

// Run test t and return the result
float benchmarkRun(int t) {
  switch (t) {
    case 0:  return testInt();
    case 1:  return testFloat();
    case 2:  return testDouble();
    case 3:  return testFloat();
    case 4:  return testTwoCores();
    case 5:  return testVector(false);
    case 6:  return testVector(true);
    case 7:  return testCopy(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, 32 * 1024);
    case 8:  return testCopy(MALLOC_CAP_SPIRAM, 1024 * 1024);   // bigger than the cache: real PSRAM
    case 9:  return testFlash();
    case 10: return testSha();
    case 11: return testAes();
    case 12: return testRandom();
    default: return 0;
  }
}

// /benchmark?t=3  ->  run test 3 and send the result
void handleBenchmark() {
  requestCounter++;
  int t = server.hasArg("t") ? server.arg("t").toInt() : -1;
  if (t < 0 || t >= BENCH_COUNT) {
    server.send(404, "application/json", "{\"error\":\"no test\"}");
    return;
  }
  float w = benchmarkRun(t);
  String j = "{\"t\":" + String(t);
  j += ",\"id\":\"" + String(BENCH_ID[t]) + "\"";
  j += ",\"name\":\"" + String(BENCH_NAME[t]) + "\"";
  j += ",\"unit\":\"" + String(BENCH_UNIT[t]) + "\"";
  j += ",\"group\":\"" + String(BENCH_GROUP[t]) + "\"";
  j += ",\"value\":" + String(w, 2);
  j += ",\"last\":";
  j += (t == BENCH_COUNT - 1) ? "true" : "false";
  j += "}";
  server.send(200, "application/json", j);
}
