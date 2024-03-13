#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Hospot Agung"
#define WIFI_PASSWORD "123456789"

#define API_KEY "AIzaSyDahGy8vNxx3i62p7MIReZry8C5lTbbS5A"

#define DATABASE_URL "https://kelompok1-iot-891c8-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27 (from DIYables LCD), 16 column and 2 rows
#define TRIG_PIN D8 
#define ECHO_PIN D7 
#define nilaiA0 A0
#define LED_PIN D5

long duration;
float duration_us, distance_cm;
const int pintuPin = D6;
const int ledPin = D5;

Servo myServo;

void setup() {
  Serial.begin(9600);
  pinMode(nilaiA0, INPUT );
  lcd.init();               // initialize the lcd
  lcd.backlight();          // open the backlight
  pinMode(TRIG_PIN, OUTPUT); // config trigger pin to output mode
  pinMode(ECHO_PIN, INPUT);  // config echo pin to input mode
  myServo.attach(pintuPin);
  pinMode(ledPin, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP : ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Sign Success");
    signupOK = true;
  } else {
    Serial.println("Sign Failed");
    Serial.println(config.signer.signupError.message.c_str());
  }

   // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int sensorValue = analogRead(nilaiA0);
  digitalWrite(TRIG_PIN, HIGH); // generate 10-microsecond pulse to TRIG pin
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration_us = pulseIn(ECHO_PIN, HIGH); // measure duration of pulse from ECHO pin

  // calculate the distance
  distance_cm = 0.017 * duration_us;

  lcd.setCursor(0, 0);
  lcd.print("Soil: ");
  lcd.print(sensorValue);

  lcd.setCursor(0, 1); // start to print at the first row
  lcd.print("Distance: ");
  lcd.print(distance_cm);

  if (sensorValue > 450) {
    myServo.write(180);
    lcd.print(sensorValue);
    delay(1000);
  } else {
    lcd.print(sensorValue);
    myServo.write(0);
    delay(1000);
  }

  if (distance_cm > 100) {
    lcd.print(distance_cm);
    Serial.println("Pompa Air menyala");
    digitalWrite(ledPin, HIGH);
    delay(1000);
  } else {
    lcd.print(distance_cm);
    Serial.println("Pompa Air mati");
    digitalWrite(ledPin, LOW);
    delay(1000);
  }

   if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    // Since we want the data to be updated every second
    sendDataPrevMillis = millis();

    // Soil-Moisture
    if (Firebase.RTDB.setInt(&fbdo, "Soil-Moisture", sensorValue)) {
      Serial.print("Moisture : ");
      Serial.println(sensorValue);
    } else {
      Serial.println("Failed to read the sensor");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    // Ultrasonic
    if (Firebase.RTDB.setFloat(&fbdo, "Ultrasonik", distance_cm)) {
      Serial.print("Jarak : ");
      Serial.println(distance_cm);
    } else {
      Serial.println("Failed to read the sensor");
      Serial.println("Reason: " + fbdo.errorReason());
    }
  }
}
