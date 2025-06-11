#include <DHT.h>
#include <HardwareSerial.h>

// ==== Cấu hình ==== 
#define NODE_ID 2
#define DHT_PIN 32
#define DHT_TYPE DHT11

#define RELAY_PUMP 18
#define RELAY_LIGHT 19   

#define SOIL_PIN 35
#define LDR_PIN 34

#define LORA_RX 26
#define LORA_TX 25

HardwareSerial LoRaSerial(2);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long previousMillis = 0;
const unsigned long interval = 4500;  // 5 giây

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_LIGHT, OUTPUT);

  digitalWrite(RELAY_PUMP, HIGH);   // Tắt
  digitalWrite(RELAY_LIGHT, HIGH);  // Tắt

  dht.begin();
  Serial.println("ESP32 bắt đầu gửi dữ liệu qua LoRa (UART2)...");
}

// ==== Hàm chuẩn hóa ADC về phần trăm ==== 
int readAnalogPercent(int pin) {
  int raw = analogRead(pin);
  return (raw * 100) / 4095;  // ESP32 có độ phân giải ADC 12-bit
}

// ==== Đọc cảm biến DHT11 ==== 
void readDHT11(float &temp, float &humi) {
  temp = dht.readTemperature();
  humi = dht.readHumidity();

  if (isnan(temp) || isnan(humi)) {
    temp = -1;
    humi = -1;
  }
}

// ==== Đọc ánh sáng (chuẩn hóa về %) ==== 
int readLightPercent() {
  delay(10);
  return readAnalogPercent(LDR_PIN);
}

// ==== Đọc độ ẩm đất (chuẩn hóa về %) ==== 
int readSoilPercent() {
  delay(10);
  return readAnalogPercent(SOIL_PIN);
}

// ==== Gửi dữ liệu qua UART2 ==== 
void sendDataUART(float temp, float humi, int lightPercent, int soilPercent) {
  LoRaSerial.print(NODE_ID); LoRaSerial.print(",");
  LoRaSerial.print(temp, 2); LoRaSerial.print(",");
  LoRaSerial.print(humi, 2); LoRaSerial.print(",");
  LoRaSerial.print(lightPercent); LoRaSerial.print(",");
  LoRaSerial.println(soilPercent);

  Serial.print("[ĐÃ GỬI] Node "); Serial.print(NODE_ID);
  Serial.print(" | Temp: "); Serial.print(temp);
  Serial.print(" | Humi: "); Serial.print(humi);
  Serial.print(" | Light: "); Serial.print(lightPercent); Serial.print("%");
  Serial.print(" | Soil: "); Serial.print(soilPercent); Serial.println("%");
}

// ==== Xử lý lệnh từ gateway ==== 
void handleSerialCommand(String cmd) {
  cmd.trim();
  Serial.print("[ĐÃ NHẬN] Lệnh từ Gateway: ");
  Serial.println(cmd);

  if (cmd.length() >= 5 && cmd.charAt(0) == 'N') {
    int cmdNodeId = cmd.substring(1, 2).toInt();
    String deviceType = cmd.substring(2, 3);
    int state = cmd.substring(4).toInt();

    if (cmdNodeId == NODE_ID) {
      if (deviceType == "L") {
        digitalWrite(RELAY_LIGHT, state ? LOW : HIGH);
        Serial.println("[THỰC THI] Đèn: " + String(state ? "BẬT" : "TẮT"));
      } else if (deviceType == "P") {
        digitalWrite(RELAY_PUMP, state ? LOW : HIGH);
        Serial.println("[THỰC THI] Máy bơm: " + String(state ? "BẬT" : "TẮT"));
      }
    }
  }
}

// ==== Loop chính ==== 
void loop() {
  if (LoRaSerial.available()) {
    String input = LoRaSerial.readStringUntil('\n');
    handleSerialCommand(input);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temperature = 0, humidity = 0;
    int lightPercent = 0, soilPercent = 0;

    readDHT11(temperature, humidity);
    lightPercent = readLightPercent();
    soilPercent = readSoilPercent();

    if (temperature != -1 && humidity != -1) {
      sendDataUART(temperature, humidity, lightPercent, soilPercent);
    } else {
      Serial.println("[CẢNH BÁO] Không đọc được dữ liệu từ DHT11");
    }
  }
}