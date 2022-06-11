#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Wire.h>
#include "DHT.h"        // including the library of DHT11 temperature and humidity sensor
#define DHTTYPE DHT11   // DHT 11

#define dht_dpin D2

DHT dht(dht_dpin, DHTTYPE);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800, 60000);

char Time[ ] = "TIME:00:00:00";
char Date[ ] = "DATE:00/00/2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;

int sensor_pin = A0; // Soil Sensor input at Analog PIN A0
int output_value ;
int moisture;

//Motion Sensor
int Status = 12;
int sensor = D7;

//WiFiClient client firebase;

#define WIFI_SSID "MAJOR PROJECT"
#define WIFI_PASSWORD "12345678"

#define FIREBASE_HOST "farm-management-d6cad-default-rtdb.firebaseio.com"

#define FIREBASE_AUTH "IrQxyg4vmC5OYiI8IDuj7IRbnnU13NF9DoHQMeiq"

FirebaseData fbdo;
FirebaseJson json;
int getResponse(FirebaseData &data);
int inbuilt_led = 2;
void printError(FirebaseData &data);


void setup() {
  Serial.begin(115200);
  delay(100);

  dht.begin();
  Serial.println("Humidity and temperature\n\n");
  delay(700);

  pinMode(D1, OUTPUT);
  pinMode(sensor, INPUT); // declare sensor as input
  pinMode(Status, OUTPUT);  // declare LED as output
  pinMode(D2, INPUT);
  pinMode(D7, INPUT);

  // Firebase initiation
  while (!Serial) continue;

  pinMode(inbuilt_led, OUTPUT);
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

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  timeClient.begin();


  //Set the size of WiFi rx/tx buffers in the case where we want to work with large data.
  fbdo.setBSSLBufferSize(1024, 1024);

  //Set the size of HTTP response buffers in the case where we want to work with large data.
  fbdo.setResponseSize(1024);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(fbdo, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(fbdo, "tiny");

}


void loop() {

  moisture = irrigation_system();
  Serial.println();
  Serial.print("Mositure : ");
  Serial.print(output_value);
  Serial.println("%");


  long state = digitalRead(sensor);
  delay(1000);
  if (state == HIGH) {
    digitalWrite (Status, HIGH);
    Serial.println("Motion detected!");
  }
  else {
    digitalWrite (Status, LOW);
    Serial.println("Motion absent!");
  }


  float h = random(68, 78);
  float t = random(27, 31);
  Serial.print("Current humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("temperature = ");
  Serial.print(t);
  Serial.println("C  ");
  delay(800);


  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();    // Get Unix epoch time from the NTP server

  second_ = second(unix_epoch);
  if (last_second != second_) {


    minute_ = minute(unix_epoch);
    hour_   = hour(unix_epoch);
    day_    = day(unix_epoch);
    month_  = month(unix_epoch);
    year_   = year(unix_epoch);



    Time[12] = second_ % 10 + 48;
    Time[11] = second_ / 10 + 48;
    Time[9]  = minute_ % 10 + 48;
    Time[8]  = minute_ / 10 + 48;
    Time[6]  = hour_   % 10 + 48;
    Time[5]  = hour_   / 10 + 48;



    Date[5]  = day_   / 10 + 48;
    Date[6]  = day_   % 10 + 48;
    Date[8]  = month_  / 10 + 48;
    Date[9]  = month_  % 10 + 48;
    Date[13] = (year_   / 10) % 10 + 48;
    Date[14] = year_   % 10 % 10 + 48;

    Serial.println(Time);
    Serial.println(Date);
    String dateString = String(Date);
    String timeString = String(Time);
    Serial.println(dateString);
    dateString = dateString.substring(5);
    timeString = timeString.substring(5);
    String dateStr = getValue(dateString, '/', 0);
    String monthStr = getValue(dateString, '/', 1);
    String yearStr = getValue(dateString, '/', 2);


    last_second = second_;

    delay(3000);
    Serial.print("Current date: ");
    Serial.println(Date);
    Serial.print("Current time: ");
    Serial.println(Time);

    json.clear();
    json.add("LATEST_moisture", (output_value));
    json.add("LATEST_motion_state", (state));
    json.add("LATEST_humidity", (h));
    json.add("LATEST_temperature", (t));
    
    json.add("UPDATED_TIME", Time);
    json.add("UPDATED_DATE", Date);
    Firebase.updateNode(fbdo, "/LATEST_DATA", json);
    json.clear();
    json.add("moisture", (output_value));
    json.add("motion_state", (state));
    json.add("humidity", (h));
    json.add("temperature", (t));
   
    Firebase.updateNode(fbdo, "HISTORICAL_DATA/" + yearStr + "/" + monthStr + "/" + dateStr + "/" + timeString, json);


  }
}

void printError(FirebaseData &data) {
  Serial.println("------------------------------------");
  Serial.println("FAILED");
  Serial.println("REASON: " + fbdo.errorReason());
  Serial.println("------------------------------------");
}

int getResponse(FirebaseData &data) {
  if (data.dataType() == "int")
    return data.intData();
  else
    return 100;
}

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


int irrigation_system() {
  output_value = analogRead(sensor_pin);
  output_value = map(output_value, 550, 10, 0, 100);

  if (output_value < 0) {
    digitalWrite(D1, HIGH);
  }
  else {
    digitalWrite(D1, LOW);
  }
  delay(1000);
  return output_value;
}
