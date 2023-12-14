#define BLYNK_TEMPLATE_ID "TMPL6JKE4h82Y"
#define BLYNK_TEMPLATE_NAME "Mr Siro"
#define BLYNK_AUTH_TOKEN "Go-nMU-rvSfVjBNvtRn2RV7kLVMsy5dl"

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <Servo.h>

const int servoPin = D3;
Servo servo;

const char* ssid = "hihi";
const char* password = "0902030601";
const char* apiKey = "94123a1acaf64458aac3fdab9be1dff1";  // Replace with your OpenWeatherMap API key
const char* city = "Ha Noi";                              // Replace with the city you want to get the weather for

const char* host = "api.openweathermap.org";
const int httpsPort = 80;  // OpenWeatherMap supports HTTP on port 80

#define led 2
#define sensor A0

LiquidCrystal_I2C lcd(0x27, 16, 2);

float light_intensity;
float temperature;
float humidity;
bool isShowHumidity = true;
String condition;
String sunrise;
String sunset;
int showDataOrder = 0;

int ledState = 0;
int autoState = 0;

const int timeZoneOffset = 7;  // Replace with your desired time zone offset in hours

unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;
const unsigned long interval = 43200000;  //// Call the API every 12 hour (in milliseconds)
const unsigned long interval2 = 3000;     //// Update text every 3 second (in milliseconds)
void setup() {

  Serial.begin(9600);
  delay(10);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  lcd.clear();
  lcd.print("WiFi connected");
  Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
  Blynk.connect();
  Blynk.virtualWrite(V8, HIGH);
  pinMode(led, OUTPUT);
  pinMode(sensor, INPUT);
  createHttpRequest();

  servo.attach(servoPin);
}

void loop() {
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the current time
    previousMillis = currentMillis;

    // Call the API
    createHttpRequest();
  }
  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= interval2) {
    previousMillis2 = currentMillis2;
    switch (showDataOrder) {
      case 0:
        {
          lcd.setCursor(0, 1);
          lcd.print("Humidity : ");
          lcd.setCursor(11, 1);
          lcd.print(humidity);
          isShowHumidity = false;
          break;
        }
      case 1:
        {
          lcd.setCursor(0, 1);
          lcd.print("Temp     : ");
          lcd.setCursor(11, 1);
          lcd.print(temperature);
          isShowHumidity = true;
          break;
        }
      case 2:
        {
          lcd.setCursor(0, 1);
          lcd.print("Condition: ");
          lcd.setCursor(11, 1);
          lcd.print(condition);
          isShowHumidity = true;
          break;
        }
      case 3:
        {
          lcd.setCursor(0, 1);
          lcd.print("Sunrise  : ");
          lcd.setCursor(11, 1);
          lcd.print(sunrise);
          Serial.println(sunrise);
          break;
        }
      case 4:
        {
          lcd.setCursor(0, 1);
          lcd.print("Sunset   : ");
          lcd.setCursor(11, 1);
          lcd.print(sunset);
          Serial.println(sunset);
          break;
        }
      default:
        {
          break;
        }
    }
    if (showDataOrder == 4) {
      showDataOrder = 0;
    } else {
      showDataOrder++;
    }
  }
  // Nothing to do here
  Blynk.run();
  readSensor();
  if (light_intensity < 20) {
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }
  // Control the servo motor based on light intensity
  if (light_intensity < 20) {
    servo.write(0);  // Rotate the servo to 0 degrees
  } else {
    servo.write(180);  // Rotate the servo to 180 degrees
  }
}

BLYNK_CONNECTED() {
  Blynk.syncAll();

}

void readSensor() {
  int value = 1023 - analogRead(sensor);
  light_intensity = map(value, 0, 1023, 0, 100);
  lcd.setCursor(0, 0);
  lcd.print("Intensity: ");
  lcd.setCursor(11, 0);
  lcd.print(light_intensity);
  Blynk.virtualWrite(V5, light_intensity);
  delay(100);
}

void createHttpRequest() {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(apiKey) + "&mode=json&units=metric&cnt=2";
  WiFiClient client;
  lcd.setCursor(0, 0);
  lcd.print("Connecting API");
  http.begin(client, url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, http.getString());

    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      lcd.setCursor(0, 0);
      lcd.print("API error");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("API connected");
      // Access weather data
      condition = doc["weather"][0]["main"].as<String>();
      temperature = doc["main"]["temp"];
      humidity = doc["main"]["humidity"];
      sunrise = utcToHumanReadable(doc["sys"]["sunrise"]);
      sunset = utcToHumanReadable(doc["sys"]["sunset"]);
      Blynk.virtualWrite(V2, condition);
      Blynk.virtualWrite(V3, sunrise);
      Blynk.virtualWrite(V4, sunset);
      Blynk.virtualWrite(V6, humidity);
      Blynk.virtualWrite(V7, temperature);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("API error");
  }
  http.end();
}

String utcToHumanReadable(long utc) {
  struct tm* timeinfo;
  time_t rawtime = (time_t)utc;
  timeinfo = localtime(&rawtime);
  int hour = timeinfo->tm_hour + timeZoneOffset;
  if (hour > 23) {
    hour = hour - 24;
  }
  String data = String(hour);
  data = data + ":";
  data = data + String(timeinfo->tm_min);
  data = data + " ";
  return data;
}