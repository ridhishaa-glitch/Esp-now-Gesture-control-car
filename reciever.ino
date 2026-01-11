#include <esp_now.h>
#include <WiFi.h>

// --- PIN DEFINITIONS ---
const int F_IN4 = 14; const int F_IN3 = 27; 
const int F_IN2 = 26; const int F_IN1 = 25;
const int B_IN4 = 5;  const int B_IN3 = 18; 
const int B_IN2 = 19; const int B_IN1 = 21;

// Smoothing Factor (0.05 - 0.20 for smooth glide)
const float LERP_FACTOR = 0.12; 

struct Packet {
  int16_t x;
  int16_t y;
  int16_t turn;
};
Packet incoming;
unsigned long lastMsg = 0;

// Current motor speeds for LERP
float curFL = 0, curFR = 0, curBL = 0, curBR = 0;

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  memcpy(&incoming, data, sizeof(incoming));
  lastMsg = millis();
}

void setup() {
  const int pins[] = {14, 27, 26, 25, 5, 18, 19, 21};
  for(int p : pins) {
    pinMode(p, OUTPUT);
    analogWrite(p, 0);
  }

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void drive(int p1, int p2, float speed) {
  int pwm = constrain((int)speed, -255, 255);
  if (pwm > 15) { analogWrite(p1, pwm); analogWrite(p2, 0); }
  else if (pwm < -15) { analogWrite(p1, 0); analogWrite(p2, abs(pwm)); }
  else { analogWrite(p1, 0); analogWrite(p2, 0); }
}

void loop() {
  // Failsafe: Stop if signal lost
  if (millis() - lastMsg > 400) {
    incoming.x = 0; incoming.y = 0; incoming.turn = 0;
  }

  // 1. Mecanum Math
  float tFL = (float)incoming.y + incoming.x + incoming.turn;
  float tFR = (float)incoming.y - incoming.x - incoming.turn;
  float tBL = (float)incoming.y - incoming.x + incoming.turn;
  float tBR = (float)incoming.y + incoming.x - incoming.turn;

  // 2. Normalization (Keeps values <= 255 and prevents crashes)
  float maxVal = max(abs(tFL), max(abs(tFR), max(abs(tBL), max(abs(tBR)))));
  if (maxVal > 255.0) {
    tFL = (tFL / maxVal) * 255.0; tFR = (tFR / maxVal) * 255.0;
    tBL = (tBL / maxVal) * 255.0; tBR = (tBR / maxVal) * 255.0;
  }

  // 3. Smoothing (Interpolation)
  curFL += (tFL - curFL) * LERP_FACTOR;
  curFR += (tFR - curFR) * LERP_FACTOR;
  curBL += (tBL - curBL) * LERP_FACTOR;
  curBR += (tBR - curBR) * LERP_FACTOR;

  // 4. Update Motors
  drive(F_IN1, F_IN2, curFL);
  drive(F_IN3, F_IN4, curFR);
  drive(B_IN1, B_IN2, curBL);
  drive(B_IN3, B_IN4, curBR);

  delay(10); // Processor yield
}

