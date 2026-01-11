#include <esp_now.h>
#include <WiFi.h>
#include "I2Cdev.h"
#include "MPU6050.h"

// MAC Address of your Receiver
uint8_t receiverAddress[] = {};

struct Packet {
  int16_t x;
  int16_t y;
  int16_t turn;
};
Packet myControlData;

MPU6050 mpu;
esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize and Calibrate MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    while(1) { Serial.println("MPU6050 Fail"); delay(1000); }
  }
  
  // Your requested PID-based calibration
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);

  // WiFi & ESP-NOW Setup
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;

  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Map Tilt to Speed (-255 to 255)
  // X: Side-to-side (Roll), Y: Forward-back (Pitch)
  int16_t rawX = map(ay, -17000, 17000, -255, 255);
  int16_t rawY = map(ax, -17000, 17000, 255, -255);
  int16_t rawTurn = map(gz, -17000, 17000, -255, 255);

  // Deadzone to eliminate jitter while hand is flat
  myControlData.x = (abs(rawX) < 35) ? 0 : rawX;
  myControlData.y = (abs(rawY) < 35) ? 0 : rawY;
  myControlData.turn = (abs(rawTurn) < 45) ? 0 : rawTurn;

  // Print data for debugging
  Serial.print("X: "); Serial.print(myControlData.x);
  Serial.print(" Y: "); Serial.print(myControlData.y);
  Serial.print(" T: "); Serial.println(myControlData.turn);

  esp_now_send(receiverAddress, (uint8_t *) &myControlData, sizeof(myControlData));
  delay(20); 
}
