#include <Arduino.h>
#include <CheapStepper.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHT_TYPE DHT22
#define DHT_PIN 2
#define LED_PIN 13
#define HEAT_SWITCH_PIN 4
#define LIGHT_SWITCH_PIN 5
#define OPTIMAL_TEMPERATURE 37.7f

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0.0f;
float humidity = 0.0f;

CheapStepper stepper (8,9,10,11);  
bool moveClockwise = true;

void setupHeater()
{
  pinMode(HEAT_SWITCH_PIN, OUTPUT);
  digitalWrite(HEAT_SWITCH_PIN, LOW);
  pinMode(LIGHT_SWITCH_PIN, OUTPUT);
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
}

void setupStepper()
{
  stepper.setRpm(12);
  stepper.newMoveTo(moveClockwise, 2048);
}

void moveStepper()
{
  stepper.run();
  int stepsLeft = stepper.getStepsLeft();

  if (stepsLeft == 0){
    moveClockwise = !moveClockwise; // reverse direction
    stepper.newMoveDegrees (moveClockwise, 180); // move 180 degrees from current position
  }
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
  data.printTo(Serial);
  Serial.println();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up");
  setupSensor();
  //setupStepper();
  setupHeater();
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

void loop()
{
  readSensor();
  //moveStepper();
  handleHeat();
  publishStats();
  delay(10000);
}