/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp32-data-logging-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"
#include "sntp.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "PLDTHOMEFIBRe3638"
#define WIFI_PASSWORD "PLDTWIFIjbx63"

// Insert Firebase project API Key
// #define API_KEY "AIzaSyCWZ0VcS-8Cjh0exCpIUuzx9czdUJPLefA"
#define API_KEY "AIzaSyAQPF6BwI3VgGgl21v_TIWnd8vC8i79zBA"


// Insert Authorized Email and Corresponding Password
 #define USER_EMAIL "adventi098@gmail.com"
 #define USER_PASSWORD "@Ponsii098"
// #define USER_EMAIL "admin@admin.com"
// #define USER_PASSWORD "123456"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://testing-f4b02-default-rtdb.asia-southeast1.firebasedatabase.app/"
//#define DATABASE_URL "https://coms-stateled-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String consumptionPath = "/consumption";
// Parent Node (to be updated in every loop)
String parentPath;

//int timestamp;
String timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000;

// For configuring Time
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;
//const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
String getTime() {
  int now[3];
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return("Not yet obtained");
  }
  now[0] = timeinfo.tm_hour - 12;
  now[1] = timeinfo.tm_min;
  now[2] = timeinfo.tm_sec;
  return String(timeinfo.tm_mon + 1) + "-" + String(timeinfo.tm_mday) + "-" + String(timeinfo.tm_year - 100) + " " +
        String(now[0]) + ":" + String(now[1]) + ":" + String(now[2]);

}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  getTime();
}

int value = 0;

void setup(){
  Serial.begin(115200);

  // Initialize BME280 sensor
  initWiFi();
  sntp_set_time_sync_notification_cb( timeavailable );
  sntp_servermode_dhcp(1);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;
}

void loop(){
  int solenoidState;
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);
    parentPath = databasePath;
    
    json.set(consumptionPath, String(random(10, 1000)));
    json.set("/timeStamp", timestamp);
    //json.set("/solenoidStatus", 0);
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath, &json) ? "ok" : fbdo.errorReason().c_str());
    
    if(Firebase.RTDB.getJSON(&fbdo, parentPath + "/solenoidStatus", &solenoidState)){
      Serial.print("Solenoid state is: ");
      Serial.println(solenoidState);
    }else {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
}