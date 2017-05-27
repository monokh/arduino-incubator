#include <Arduino.h>
#include <CheapStepper.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Timer.h>

#define DHT_TYPE DHT22
#define DHT_PIN 2
#define LED_PIN 13
#define HEAT_SWITCH_PIN 4
#define LIGHT_SWITCH_PIN 5
#define OPTIMAL_TEMPERATURE 37.2f
#define TURNER_BACK_MEMORY_ADDRESS 256
#define TURNER_INTERVAL 10800000
#define CHECK_INTERVAL 10000
#define TURNER_DEGREES_DISTANCE 130

Timer t;

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0.0f;
float humidity = 0.0f;

CheapStepper stepper (8,9,10,11);  
bool turnerBack = true;

void setupHeater()
{
  pinMode(HEAT_SWITCH_PIN, OUTPUT);
  digitalWrite(HEAT_SWITCH_PIN, LOW);
  pinMode(LIGHT_SWITCH_PIN, OUTPUT);
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
}

void moveTurner()
{
  turnerBack = !turnerBack; // reverse direction
  EEPROM.put(TURNER_BACK_MEMORY_ADDRESS, turnerBack);
  stepper.moveDegrees(turnerBack, TURNER_DEGREES_DISTANCE); 
}

void setupTurner()
{
  stepper.setRpm(14);
  EEPROM.get(TURNER_BACK_MEMORY_ADDRESS, turnerBack);
  t.every(TURNER_INTERVAL, moveTurner);
}

void setupSensor() 
{
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void readSensor()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

void publishStats()
{
  StaticJsonBuffer<200> json;
  JsonObject& data = json.createObject();
  data["temperature"] = temperature;
  data["humidity"] = humidity;
  data["runtime"] = millis();
  data["turner"] = turnerBack ? "back" : "forward";
  data.printTo(Serial);
  Serial.println();
}

void handleHeat()
{
  if (temperature < OPTIMAL_TEMPERATURE) {
    digitalWrite(HEAT_SWITCH_PIN, HIGH);
    digitalWrite(LIGHT_SWITCH_PIN, LOW);
  } else {
    digitalWrite(HEAT_SWITCH_PIN, LOW);
    digitalWrite(LIGHT_SWITCH_PIN, HIGH);
  }
}

void check() 
{
  readSensor();
  handleHeat();
  publishStats();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up");
  setupSensor();
  setupTurner();
  setupHeater();
  t.every(CHECK_INTERVAL, check);
}

void loop()
{
  t.update();
}