#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <Wire.h>
#include <OneWire.h>
#include <time.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DHTPIN 25
#define DHTTYPE DHT11

#define WIFI_SSID "******"
#define WIFI_PASSWORD "*******"

#define API_KEY " ****************   "
#define DATABASE_URL " ******** "

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long getDataPrevMillis = 0;
int count = 1;
bool signupOK = false;
String intValue;

int alarmLed = 27;
int alarmSesi = 12;
int kapiSensPin = 33;
int yanginSensPin = 32;
int sensValue = 34;
DHT dht(DHTPIN, DHTTYPE);
int gasSensorPin = 13;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected. IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign up successful");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign up error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
   pinMode(sensValue, INPUT);
  pinMode(kapiSensPin, INPUT);
  pinMode(yanginSensPin, INPUT);
  pinMode(alarmLed, OUTPUT);
  pinMode(alarmSesi, OUTPUT);
  noTone(alarmSesi);
  dht.begin();

  ledcSetup(0, 5000, 8);
  ledcAttachPin(alarmSesi, 0);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "GMT-3", 1);
}

void logEvent(String eventType) {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

  if (Firebase.ready() && signupOK) {
    String path = "/EVENT_LOGS/" + eventType + "/" + String(now);
    if (!Firebase.RTDB.setString(&fbdo, path + "/time", timeStr)) {
      Serial.println("Failed to log event time: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setString(&fbdo, path + "/event", eventType)) {
      Serial.println("Failed to log event type: " + fbdo.errorReason());
    }
  }
}

void loop() {
  bool alarmTriggered = false;

  int lastValue = digitalRead(sensValue);
  Serial.print("Vibration Sensor Value: ");
  Serial.println(lastValue);
  delay(250);

  int kapiValue = digitalRead(kapiSensPin);
  Serial.print("Kapi: ");
  Serial.println(kapiValue);
  delay(200);
  if (kapiValue == 1) {
    Serial.println("Kapi Acildi!");
    logEvent("Kapi Acildi");
    alarmTriggered = true;
  }

  int yanginValue = digitalRead(yanginSensPin);
  Serial.print("Yangin degeri: ");
  Serial.println(yanginValue);
  delay(200);
  if (yanginValue == 0) {
    Serial.println("YANGIN VAR!!");
    logEvent("Yangin Var");
    alarmTriggered = true;
  }

  int gasValue = digitalRead(gasSensorPin);
  Serial.print("Gaz değeri: ");
  Serial.println(gasValue);
  delay(200);
  if (gasValue == 1) {
    Serial.println("GAZ ALGILANDI");
    logEvent("Duman algilandi");
    alarmTriggered = true;
  }

  if (alarmTriggered) {
    digitalWrite(alarmLed, HIGH);
    ledcWriteTone(0, 523);
  } else {
    digitalWrite(alarmLed, LOW);
    ledcWriteTone(0, 0);
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/SICAKLIK", String(t, 1))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/NEM", String(h, 1))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/GAZ", String(gasValue))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/YANGIN", String(yanginValue))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/KAPI", String(kapiValue))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }

    if (!Firebase.RTDB.setString(&fbdo, "/SENSOR_IZLEME/TITRESIM", String(lastValue))) {
      Serial.println("Firebase hatası: " + fbdo.errorReason());
    }
  }
}
