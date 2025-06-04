#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// Firebase 관련 헤더 파일
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "iPhone (103)"
#define WIFI_PASSWORD "12311231"


#define API_KEY "AIzaSyCeSDaBkr1tmSSyGdFxj20sCaZ_bzXMYvE"
#define DATABASE_URL "https://smartfarm-35bb7-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
int bufferIndex = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase 연결 성공");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // 시간 설정
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time sync");
  while (time(nullptr) < 1510644967) {
    delay(10);
  }
}

// 16비트 데이터 파싱용 함수 (두 바이트를 결합해 16비트 정수로 변환)
uint16_t parse16BitData(char highByte, char lowByte) {
  return (uint16_t)((highByte << 8) | lowByte);
}

// 8비트 데이터 파싱용 함수
uint8_t parse8BitData(char byte) {
  return (uint8_t)byte;
}

// 패킷을 그대로 출력하는 함수
void printRawData(char *data, int length) {
  Serial.print("Received raw data: ");
  for (int i = 0; i < length; i++) {
    Serial.printf("%02X ", (uint8_t)data[i]);
  }
  Serial.println();
}

void parseAndSendData(char *data) {
  printRawData(data, 17);

  if (data[0] == 0x01 && data[1] == 0x02) {
    float soilHumidity = parse16BitData(data[2], data[3]) / 10.0;
    float soilTemperature = parse16BitData(data[4], data[5]) / 10.0;
    float soilConductivity = parse16BitData(data[6], data[7]) / 10.0;
    float soilPH = parse16BitData(data[8], data[9]) / 10.0;
    float dhtHumidity = parse8BitData(data[10]) + parse8BitData(data[11]) / 100.0;
    float dhtTemperature = parse8BitData(data[12]) + parse8BitData(data[13]) / 100.0;
    uint16_t crcReceived = parse16BitData(data[14], data[15]);

    if (data[16] == 0xF0) {
      Serial.printf("Parsed Data - Soil Humidity: %.1f, Soil Temperature: %.1f, Soil Conductivity: %.1f, Soil pH: %.1f, DHT Humidity: %.2f, DHT Temperature: %.2f\n",
                    soilHumidity, soilTemperature, soilConductivity, soilPH, dhtHumidity, dhtTemperature);

      if (Firebase.ready() && signupOK) {
        // 현재 시간 가져오기
        time_t now = time(nullptr);
        struct tm* p_tm = localtime(&now);
        char dateTime[30];
        strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", p_tm);

        // Firebase에 데이터 저장
        FirebaseJson json;
        json.set("timestamp", dateTime);
        json.set("soil_humidity", soilHumidity);
        json.set("soil_temperature", soilTemperature);
        json.set("soil_conductivity", soilConductivity);
        json.set("soil_ph", soilPH);
        json.set("dht_humidity", dhtHumidity);
        json.set("dht_temperature", dhtTemperature);

        if (Firebase.RTDB.pushJSON(&fbdo, "sensor_logs", &json)) {
          Serial.println("데이터 로그 저장 성공");
        } else {
          Serial.println("데이터 로그 저장 실패");
          Serial.println("사유: " + fbdo.errorReason());
        }
      }
    } else {
      Serial.println("잘못된 패킷: 마지막 바이트가 0xF0이 아님");
      memset(data, 0, BUFFER_SIZE);
      bufferIndex = 0;
    }
  } else {
    Serial.println("잘못된 패킷: Address 또는 Function 값이 다름");
    memset(data, 0, BUFFER_SIZE);
    bufferIndex = 0;
  }
  
  memset(data, 0, BUFFER_SIZE);
  bufferIndex = 0;
}

void loop() {
  while (Serial.available()) {
    char receivedChar = Serial.read();
    buffer[bufferIndex++] = receivedChar;
    if (bufferIndex >= 17) {
      buffer[bufferIndex] = '\0';
      parseAndSendData(buffer);
      bufferIndex = 0;
    }
  }
}
