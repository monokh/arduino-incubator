#include <Arduino.h>
#include <CheapStepper.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <IRremote.h>
#include <Timer.h>
#include <math.h>

#define IR_PIN 7
#define IR_READ_INTERVAL 500
#define IR_BLANK_CODE 4294967295
#define IR_ENABLE_LIGHT_CODE 16769565
#define IR_SWITCH_LIGHT_CODE 16753245
#define IR_MOVE_TURNER_FORWARD_CODE 16761405
#define IR_MOVE_TURNER_BACKWARD_CODE 16712445
#define IR_TURNER_CODE 16720605

#define DHT_TYPE DHT22
#define DHT_PIN 2

#define LED_PIN 13

#define HEAT_SWITCH_PIN 4
#define LIGHT_SWITCH_PIN 5
#define MINIMUM_TEMPERATURE 37.2f
#define MAXIMUM_TEMPERATURE 37.3f
#define CHECK_INTERVAL 10000

#define TURNER_BACK_MEMORY_ADDRESS 256
#define TURNER_INTERVAL 3600000
#define TURNER_DEGREES_DISTANCE 180

Timer t;

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0.0f;
float humidity = 0.0f;
boolean lightOn = false;
boolean enableLight = false;

CheapStepper turner;
bool turnerBack = true;

IRrecv irrecv(IR_PIN);
decode_results IRResult;

void setupIR()
{
  irrecv.enableIRIn();
}

void setupHeater()
{
  pinMode(HEAT_SWITCH_PIN, OUTPUT);
  digitalWrite(HEAT_SWITCH_PIN, LOW);
  pinMode(LIGHT_SWITCH_PIN, OUTPUT);
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
  lightOn = false;
}

void moveTurner()
{
  turnerBack = !turnerBack; // reverse direction
  EEPROM.put(TURNER_BACK_MEMORY_ADDRESS, turnerBack);
  turner.moveDegrees(turnerBack, TURNER_DEGREES_DISTANCE); 
}

void setupTurner()
{
  turner = CheapStepper(8,9,10,11);
  turner.setRpm(12);
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
  data["turner"] = turnerBack ? 0 : 1;
  data["light_enabled"] = enableLight ? 1 : 0;
  data.printTo(Serial);
  Serial.println();
}

void turnOffLight() 
{
  lightOn = false;
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
}

void turnOnLight()
{
  lightOn = true;
  digitalWrite(LIGHT_SWITCH_PIN, LOW);
}

void switchLight() 
{
  lightOn = !lightOn;
  digitalWrite(LIGHT_SWITCH_PIN, lightOn ? LOW : HIGH);
}

void handleHeat()
{
  if (temperature <= MINIMUM_TEMPERATURE) {
    digitalWrite(HEAT_SWITCH_PIN, HIGH);
    if(enableLight) {
      turnOnLight();
    }
  }
  else if (temperature >= MAXIMUM_TEMPERATURE) {
    digitalWrite(HEAT_SWITCH_PIN, LOW);
    turnOffLight();
  }
}

void handleCommand(unsigned long command)
{
  if(command == IR_BLANK_CODE)
    return;
    
  if(command == IR_ENABLE_LIGHT_CODE) {
    enableLight = !enableLight;
    turnOffLight();
  }
  if(command == IR_SWITCH_LIGHT_CODE) {
    switchLight();
  }
  else if (command == IR_TURNER_CODE) {
    moveTurner();
  }
  else if (command == IR_MOVE_TURNER_FORWARD_CODE) {
    turner.moveDegrees(false, 5); 
  }
  else if (command == IR_MOVE_TURNER_BACKWARD_CODE) {
    turner.moveDegrees(true, 5);
  }
  else {
    Serial.print("Received unknown command: ");
    Serial.print(command);
    Serial.println();
  }
}

void readIR()
{
  if (irrecv.decode(&IRResult)) {
    handleCommand(IRResult.value);
    irrecv.resume(); 
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
  setupIR();
  t.every(CHECK_INTERVAL, check);
  t.every(IR_READ_INTERVAL, readIR);
}

void loop()
{
  t.update();
}