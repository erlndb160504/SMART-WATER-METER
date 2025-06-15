// Blynk configuration
#define BLYNK_TEMPLATE_ID "TMPL6oY6nx3-Q"
#define BLYNK_TEMPLATE_NAME "SMART WATER METER"
#define BLYNK_AUTH_TOKEN "Hh6oCyR6STnG7AgyQGR59SfrnA2p88tB"

// Libraries
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "elmModel.h"  // Header ELM Model

// WiFi credentials
const char* ssid = "SmartWaterMeter";
const char* pass = "capstone4";

// Sensor pins for 3 taps
const byte SENSOR_PINS[3][3] = {
  {25, 27, 26},  // Tap 1: Main pipe
  {33, 32, 35},  // Tap 2: Bathroom
  {4, 18, 19}    // Tap 3: Kitchen
};

#define PRESSURE_SENSOR_PIN 34

// Pulse counters
volatile unsigned long pulseCounts[3][3] = {{0}};

// Flow and volume
float medianFlows[3] = {0.0};
float totalLiters = 0.0;
float tapLiters[3] = {0.0};
float minutelyUsage[3] = {0.0};
float lastMinutelyTotal = 0.0;

// Estimasi biaya
float waterCostPerM3 = 0.0;
float estimatedTotalCost = 0.0;

// Interval
const unsigned long READ_INTERVAL = 1000;
unsigned long lastReadTime = 0;

// Notifikasi cooldown
unsigned long lastNotificationTime = 0;
const unsigned long NOTIFICATION_COOLDOWN = 10000; // 5 menit (dalam milidetik)

BlynkTimer timer;

// Interrupt handlers
void IRAM_ATTR countTap1Sensor1() { pulseCounts[0][0]++; }
void IRAM_ATTR countTap1Sensor2() { pulseCounts[0][1]++; }
void IRAM_ATTR countTap1Sensor3() { pulseCounts[0][2]++; }
void IRAM_ATTR countTap2Sensor1() { pulseCounts[1][0]++; }
void IRAM_ATTR countTap2Sensor2() { pulseCounts[1][1]++; }
void IRAM_ATTR countTap2Sensor3() { pulseCounts[1][2]++; }
void IRAM_ATTR countTap3Sensor1() { pulseCounts[2][0]++; }
void IRAM_ATTR countTap3Sensor2() { pulseCounts[2][1]++; }
void IRAM_ATTR countTap3Sensor3() { pulseCounts[2][2]++; }

void (*interruptHandlers[3][3])() = {
  {countTap1Sensor1, countTap1Sensor2, countTap1Sensor3},
  {countTap2Sensor1, countTap2Sensor2, countTap2Sensor3},
  {countTap3Sensor1, countTap3Sensor2, countTap3Sensor3}
};

// Blynk input harga air
BLYNK_WRITE(V7) {
  waterCostPerM3 = param.asFloat();
  Serial.print("V7 diterima, harga air per m3: ");
  Serial.println(waterCostPerM3);
}

bool prevBinaryOutput[output_dim] = {0, 0, 0};

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inisialisasi pin sensor
  for (int t = 0; t < 3; t++) {
    for (int s = 0; s < 3; s++) {
      pinMode(SENSOR_PINS[t][s], INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(SENSOR_PINS[t][s]), interruptHandlers[t][s], RISING);
    }
  }

  // Inisialisasi WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Inisialisasi Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Blynk Connected!");

  // Inisialisasi timer untuk tekanan
  timer.setInterval(1000L, sendPressureToBlynk);
  Serial.println("System initialized. Connected to Blynk...");
}

void loop() {
  Blynk.run();
  timer.run();

  unsigned long currentTime = millis();
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    processSensorData();
  }
}

void processSensorData() {
  float sensorFlows[3][3];

  for (int t = 0; t < 3; t++) {
    float flows[3];
    noInterrupts();
    for (int s = 0; s < 3; s++) {
      flows[s] = pulseCounts[t][s] / 7.5; // Konversi pulsa ke laju aliran
      sensorFlows[t][s] = flows[s];
      pulseCounts[t][s] = 0;
    }
    interrupts();

    medianFlows[t] = getSmartMedian(flows[0], flows[1], flows[2]);
    tapLiters[t] += medianFlows[t] / 60.0; // Konversi ke liter
    if (t > 0) minutelyUsage[t] += medianFlows[t] / 60.0;
  }

  float totalFlow = medianFlows[1] + medianFlows[2];
  totalLiters = tapLiters[0];
  lastMinutelyTotal = minutelyUsage[1] + minutelyUsage[2];

  updateBlynk();

  // === ELM Prediction ===
  float pressure = pressure_reading();
  time_t now = millis() / 1000;
  int seconds = now % 60;
  int minutes = (now / 60) % 60;
  int hours = (now / 3600) % 24;

  float elmInput[input_dim] = {
    medianFlows[0],
    medianFlows[1],
    medianFlows[2],
    pressure
    // (float)(hours + minutes / 60.0 + seconds / 3600.0) // Uncomment jika waktu diperlukan
  };

  float elmOutput[output_dim];
  predict(elmInput, elmOutput);
  printBinaryOutput(elmOutput);
}

void updateBlynk() {
  Blynk.virtualWrite(V0, medianFlows[0]); // Main pipe flow
  Blynk.virtualWrite(V2, medianFlows[1]); // Bathroom flow
  Blynk.virtualWrite(V3, medianFlows[2]); // Kitchen flow
  Blynk.virtualWrite(V1, totalLiters);    // Total volume
  Blynk.virtualWrite(V4, tapLiters[0]);   // Main pipe volume
  Blynk.virtualWrite(V5, tapLiters[1]);   // Bathroom volume
  Blynk.virtualWrite(V6, tapLiters[2]);   // Kitchen volume

  estimatedTotalCost = totalLiters * waterCostPerM3;
  Blynk.virtualWrite(V8, estimatedTotalCost); // Estimasi biaya
}

void sendPressureToBlynk() {
  float pressure = pressure_reading();
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" bar");

  Blynk.virtualWrite(V9, pressure); // Kirim tekanan ke Blynk
}

float pressure_reading() {
  const float pressureZero = 90.4;
  const float pressureMax = 1023.6;
  const float pressureTransducerMaxBar = 6.89476;

  float pressureValue = analogRead(PRESSURE_SENSOR_PIN);
  pressureValue = ((pressureValue - pressureZero) * pressureTransducerMaxBar) / (pressureMax - pressureZero);
  
  if (pressureValue < 0.1) {
    pressureValue = 0;
  }

  return pressureValue;
}

float getSmartMedian(float a, float b, float c) {
  float values[3] = {a, b, c};
  float valid[3];
  int count = 0;
  for (int i = 0; i < 3; i++) if (values[i] > 0.01) valid[count++] = values[i];
  if (count == 0) return 0.0;
  if (count == 1) return valid[0];
  if (count == 2) return (valid[0] + valid[1]) / 2.0;
  if (valid[0] > valid[1]) swap(valid[0], valid[1]);
  if (valid[1] > valid[2]) swap(valid[1], valid[2]);
  if (valid[0] > valid[1]) swap(valid[0], valid[1]);
  return valid[1];
}

void swap(float &x, float &y) {
  float temp = x;
  x = y;
  y = temp;
}

// === ELM Prediction Functions ===
float sigmoid(float x) {
  return 1.0f / (1.0f + exp(-x));
}

void predict(const float input[input_dim], float output[output_dim]) {
  float H[hidden_dim];

  for (int j = 0; j < hidden_dim; j++) {
    H[j] = b[j];
    for (int i = 0; i < input_dim; i++) {
      H[j] += input[i] * W[i][j];
    }
    H[j] = sigmoid(H[j]);
  }

  for (int k = 0; k < output_dim; k++) {
    output[k] = 0.0f;
    for (int j = 0; j < hidden_dim; j++) {
      output[k] += H[j] * beta[j][k];
    }
    output[k] = sigmoid(output[k]);
  }
}

void printBinaryOutput(const float output[output_dim]) {
  Serial.print("Prediksi ELM: ");
  bool binaryOutput[output_dim];
  for (int i = 0; i < output_dim; i++) {
    binaryOutput[i] = output[i] > 0.6 ? 1 : 0;
    Serial.print(binaryOutput[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Cek apakah prediksi berubah
  bool changed = false;
  for (int i = 0; i < output_dim; i++) {
    if (binaryOutput[i] != prevBinaryOutput[i]) {
      changed = true;
      break;
    }
  }

  // Jika status berubah dan cooldown terpenuhi, kirim notifikasi
  unsigned long currentTime = millis();
  if (changed && currentTime - lastNotificationTime >= NOTIFICATION_COOLDOWN) {
    if (
      (binaryOutput[0] == 1 && binaryOutput[1] == 0 && binaryOutput[2] == 0) ||  // 100
      (binaryOutput[0] == 1 && binaryOutput[1] == 0 && binaryOutput[2] == 1) ||  // 101
      (binaryOutput[0] == 1 && binaryOutput[1] == 1 && binaryOutput[2] == 0)     // 110
    ) {
      Blynk.logEvent("leak_main_pipe", "Terjadi bocor di main pipe");
      Serial.println("Notifikasi: Terjadi bocor di main pipe");
    }
    else if (binaryOutput[0] == 0 && binaryOutput[1] == 1 && binaryOutput[2] == 0) {
      Blynk.logEvent("leak_kitchen", "Terjadi bocor di kitchen");
      Serial.println("Notifikasi: Terjadi bocor di kitchen");
    }
    else if (binaryOutput[0] == 0 && binaryOutput[1] == 0 && binaryOutput[2] == 1) {
      Blynk.logEvent("leak_bathroom", "Terjadi bocor di bathroom");
      Serial.println("Notifikasi: Terjadi bocor di bathroom");
    }
    else if (binaryOutput[0] == 0 && binaryOutput[1] == 0 && binaryOutput[2] == 0) {
      Blynk.logEvent("system_normal", "Sistem normal, tidak ada kebocoran");
      Serial.println("Notifikasi: Sistem normal, tidak ada kebocoran");
    }
    else if (binaryOutput[0] == 0 && binaryOutput[1] == 1 && binaryOutput[2] == 1) {
    Blynk.logEvent("leak_bathroom_kitchen", "Terjadi bocor di bathroom dan kitchen");
    Serial.println("Notifikasi: Terjadi bocor di bathroom dan kitchen");
  }

    lastNotificationTime = currentTime;
  }

  // Simpan status sekarang sebagai status sebelumnya untuk pembacaan berikutnya
 // for (int i = 0; i < output_dim; i++) {
 //   prevBinaryOutput[i] = binaryOutput[i];
  //}
}