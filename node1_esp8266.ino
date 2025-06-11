#include <DHT.h>

// ==== Cấu hình ==== 
#define NODE_ID 1              // ID của node
#define DHT_PIN D3             // GPIO0
#define DHT_TYPE DHT11

#define RELAY_PUMP D1          // GPIO5
#define RELAY_LIGHT D8         // GPIO15

#define SOIL_PIN A0            // Chân analog duy nhất

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long previousMillis = 0;
const unsigned long interval = 5000;  // 5 giây

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_LIGHT, OUTPUT);

  digitalWrite(RELAY_PUMP, HIGH);   // HIGH: OFF
  digitalWrite(RELAY_LIGHT, HIGH);

  dht.begin();

  Serial.println("ESP8266 bắt đầu gửi dữ liệu cảm biến qua UART...");
  randomSeed(analogRead(SOIL_PIN)); // Lấy giá trị ngẫu nhiên cho srand
}

// ==== Đọc DHT11 ==== 
void readDHT11(float &temp, float &humi) {
  temp = dht.readTemperature();
  humi = dht.readHumidity();

  if (isnan(temp) || isnan(humi)) {
    temp = -1;
    humi = -1;
  }
}

// ==== Giả lập ánh sáng ==== 
int readLightAnalog() {
  delay(10);
  int percent = random(70, 91);  // 70–90%
  return percent;
}

// ==== Đọc độ ẩm đất ==== 
int readSoilAnalog() {
  delay(10);
  int rawValue = analogRead(SOIL_PIN);  // 0–1023
  int percent = map(rawValue, 1023, 0, 0, 100);  // map ngược
  percent = constrain(percent, 0, 100);
  // Serial.print("ADC Soil = "); Serial.print(rawValue);
  // Serial.print(" → Soil % = "); Serial.println(percent);
  return percent;
}

// ==== Gửi dữ liệu cảm biến ==== 
void sendDataUART(float temp, float humi, int lightPercent, int soilPercent) {
  Serial.print(NODE_ID); Serial.print(",");
  Serial.print(temp, 2);  Serial.print(",");
  Serial.print(humi, 2);  Serial.print(",");
  Serial.print(lightPercent); Serial.print(",");
  Serial.println(soilPercent);
}

// ==== Xử lý lệnh từ Gateway ==== 
void handleSerialCommand(String cmd) {
  cmd.trim();
  // Serial.print("[LỆNH NHẬN] "); Serial.println(cmd);

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
        Serial.println("[THỰC THI] Bơm: " + String(state ? "BẬT" : "TẮT"));
      }
    }
  }
}

// ==== Loop chính ==== 
void loop() {
  // Nhận lệnh điều khiển
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    handleSerialCommand(input);
  }

  // Gửi dữ liệu cảm biến mỗi 5 giây
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
float temperature = 0, humidity = 0;
    readDHT11(temperature, humidity);

    int soilValue = readSoilAnalog();     // Đọc cảm biến đất thực
    int lightValue = readLightAnalog();   // Dữ liệu ánh sáng giả lập

    if (temperature != -1 && humidity != -1) {
      sendDataUART(temperature, humidity, lightValue, soilValue);
    } else {
      Serial.println("[LỖI] Không đọc được DHT11!");
    }
  }
}