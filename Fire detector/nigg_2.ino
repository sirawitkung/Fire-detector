#include "arduino_secrets.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "secret.h"
#include "discordCert.h"

#define MQ2_PIN 32        // à¸à¸² Analog à¸à¸­à¸ MQ-2
#define BUZZER_PIN 16     // à¸à¸² Buzzer (à¸à¸±à¸§à¸à¸µà¹à¸à¹ Active LOW à¹à¸à¹à¸à¹à¸à¸à¸µà¹)
#define WATER_SENSOR 34   // à¸à¸² Analog à¸à¸­à¸ Water Sensor
#define PUMP_PIN 17       // à¸£à¸µà¹à¸¥à¸¢à¹à¸à¸§à¸à¸à¸¸à¸¡à¸à¸±à¹à¸¡ (Active LOW: LOW=à¹à¸à¸´à¸, HIGH=à¸à¸´à¸)

int gasValue = 0;
int gasThreshold = 1000;    // à¸à¹à¸²à¸à¹à¸² MQ-2 à¸à¸­à¸à¸à¸¸à¸à¸ªà¸¹à¸à¸à¸¥à¸­à¸ à¹à¸«à¹à¸¥à¸­à¸à¸à¸¢à¸±à¸à¹à¸à¹à¸ 800 à¸à¸±à¹à¸§à¸à¸£à¸²à¸§
int waterValue = 0;
int waterThreshold = 340;

bool gasAlertSent =false;
bool waterAlertSent =false;

unsigned long previousSensorMillis =0;
const long sensorInterval = 500;

unsigned long previousHeartBeatMillis =0;
const long heartbeatInterval = 60000;

void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(WATER_SENSOR, INPUT);

  // à¸à¸´à¸à¸à¸¸à¸à¸­à¸¢à¹à¸²à¸à¸à¹à¸­à¸ (à¹à¸à¸£à¸²à¸° HIGH = à¸à¸´à¸)
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(PUMP_PIN, HIGH);

  connectToWiFi();
  sendDiscordMessage("System Online \nGas & Water monitor is active");

  sendHeartbeat();
}

void loop() {
  unsigned long curretMillis = millis();
  
  if (curretMillis - previousSensorMillis >= sensorInterval) { 
    previousSensorMillis = curretMillis;
    sensorRead();
    gasLogic();
    waterLogic();
    Serial.println("-------------------------");
  }
  if (curretMillis - previousHeartBeatMillis >= heartbeatInterval) {
    previousHeartBeatMillis = curretMillis;
    Serial.println("HEARTBEAT: Sending Signal");
    
    sendHeartbeat();
  }
  
}

void gasLogic(){
  // à¸à¹à¸²à¹à¸à¸­à¹à¸à¹à¸ªà¸à¸£à¸´à¸ à¹ à¸à¹à¸­à¸¢à¹à¸à¸´à¸
  if (gasValue > gasThreshold) {
    digitalWrite(BUZZER_PIN, LOW);   // à¹à¸à¸´à¸ buzzer (Active LOW)
    digitalWrite(PUMP_PIN, LOW);     // à¹à¸à¸´à¸à¸à¸±à¹à¸¡ (Active LOW)
    Serial.println("Gas detected! Buzzer + Pump ON");
  if (!gasAlertSent) {
    Serial.println("GAS:Sending Discord alert..");
    String message = "GAS ALERT! \nGas value (" + String(gasValue)+ ") is over the threshold";
    sendDiscordMessage(message);
    gasAlertSent = true;
   } 
  } else {
    digitalWrite(BUZZER_PIN, HIGH);  // à¸à¸´à¸ buzzer
    digitalWrite(PUMP_PIN, HIGH);    // à¸à¸´à¸à¸à¸±à¹à¸¡
    if (gasAlertSent) {
      Serial.println("GAS: Clear Sending All clear message");
      String message = "ALL CLEAR \nGas value (" + String(gasValue)+ ") is back to normal";
      sendDiscordMessage(message);
      gasAlertSent = false;
    }
   }
  }

  void waterLogic(){
  // à¹à¸à¹à¹à¸à¸·à¸­à¸à¹à¸£à¸·à¹à¸­à¸à¸à¹à¸³
  if (waterValue < waterThreshold) {
    Serial.println("LOW WATER!");
    if (!waterAlertSent) {
      Serial.println("WATER:Sending Discord alert..");
    String message = "LOW WATER ALERT! \nWater level (" + String(waterValue)+ ") is low";
    sendDiscordMessage(message);
    waterAlertSent = true;
    }
  } else {
    if (waterAlertSent) {
      Serial.println("WATER:OK Sending Refilled message");
    String message = "WATER LEVEL OK \nWater has been refiiled";
    sendDiscordMessage(message);
    waterAlertSent = false;
    }
  }
}
void sensorRead(){
  gasValue = analogRead(MQ2_PIN);
  waterValue = analogRead(WATER_SENSOR);

  Serial.print("MQ-2 Value: ");
  Serial.println(gasValue);

  Serial.print("Water Sensor Value: ");
  Serial.println(waterValue);
}
void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("HEARTBEAT:WiFi not connected.Cant send(btw you suck)");
      return;
  }

  if (String(HEALTHCHECK_URL).length() < 10) {
    Serial.println("HEARTBEAT:URL not set.Skipping");
    return;
  }

  HTTPClient http;
  http.begin(HEALTHCHECK_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("HEARTBEAT:Signal sent.Response code:  %d\n", httpCode);
  } else {
    Serial.printf("HEARTBEAT:Failled to send.Error:  %d\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}



void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendDiscordMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("DISCORD:WiFi not connected.Cant send message(btw you suck)");
      return;
  }
  
  message.replace("\n", "\\n");
  
  WiFiClientSecure client;
  client.setCACert(DISCORD_CERT);
  HTTPClient http;
  if (http.begin(client, DISCORD_WEBHOOK)) {
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"content\":\"" + message + "\", \"tts\":" + DISCORD_TTS + "}";
     int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("DISCORD:Message sent.Response code:  %d\n", httpCode);
  } else {
    Serial.printf("DISCORD:Failed to send message.Error:  %d\s", http.errorToString(httpCode).c_str());
  }
  http.end();
  } else {
    Serial.println("DISCORD: Could not connect.");
  }
}