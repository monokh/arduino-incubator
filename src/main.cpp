#include <Arduino.h>
#include <SevSeg.h>
#include <DHT.h>
#include <EEPROM.h>
#include <IRremote.h>
#include <Timer.h>
#include <math.h>

#define IR_PIN A4
#define IR_READ_INTERVAL 500
#define IR_BLANK_CODE 4294967295 // ???
#define IR_TOGGLE_MODE_COMMAND 70

#define DHT_TYPE DHT22
#define DHT_PIN A1

#define DISPLAY_NUM_DIGITS 4
#define DISPLAY_DIGIT_PINS \
  {                        \
    2, 3, 4, 5             \
  }
#define DISPLAY_SEGMENTS_PINS  \
  {                            \
    6, 7, 8, 9, 10, 11, 12, 13 \
  }
#define DISPLAY_COMMON_TYPE COMMON_CATHODE
#define DISPLAY_BRIGHTNESS 10
#define DISPLAY_MODE_TEMPERATURE 1

#define HEAT_SWITCH_PIN A2
#define LIGHT_SWITCH_PIN A3
#define MINIMUM_TEMPERATURE 37.5f
#define MAXIMUM_TEMPERATURE 37.5f
#define CHECK_INTERVAL 10000

enum DisplayMode
{
  Temperature,
  Humidity
};

SevSeg sevseg; //Instantiate a seven segment object
Timer t;

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0.0f;
float humidity = 0.0f;
bool lightOn = false;
bool lightEnabled = true;
bool displayMode = DisplayMode::Temperature;

void setupIR()
{
  IrReceiver.begin(IR_PIN, false);
}

void setupHeater()
{
  pinMode(HEAT_SWITCH_PIN, OUTPUT);
  digitalWrite(HEAT_SWITCH_PIN, LOW);
  pinMode(LIGHT_SWITCH_PIN, OUTPUT);
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
  lightOn = false;
}

void setupSensor()
{
  dht.begin();
}

void setupDisplay()
{
  byte digitPins[] = DISPLAY_DIGIT_PINS;
  byte segmentPins[] = DISPLAY_SEGMENTS_PINS;
  sevseg.begin(DISPLAY_COMMON_TYPE, DISPLAY_NUM_DIGITS, digitPins, segmentPins);
  sevseg.setBrightness(DISPLAY_BRIGHTNESS);
}

void writeDisplay() {
  char buf[5];
  if (displayMode == DisplayMode::Temperature)
  {
    String(temperature, 2).toCharArray(buf, 10);
    buf[4] = 'C';
  } else if (displayMode == DisplayMode::Humidity) {
    String(humidity, 2).toCharArray(buf, 10);
    buf[4] = 'P';
  }
  sevseg.setChars(buf);
  sevseg.refreshDisplay();
}

void readSensor()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
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

void handleHeat()
{
  if (temperature <= MINIMUM_TEMPERATURE)
  {
    digitalWrite(HEAT_SWITCH_PIN, HIGH);
    if (lightEnabled)
    {
      turnOnLight();
    }
  }
  else if (temperature > MAXIMUM_TEMPERATURE)
  {
    digitalWrite(HEAT_SWITCH_PIN, LOW);
    turnOffLight();
  }
}

void handleCommand(unsigned long command)
{
  if (command == IR_BLANK_CODE)
    return;

  if (command == IR_TOGGLE_MODE_COMMAND)
  {
    displayMode = displayMode == DisplayMode::Temperature ? DisplayMode::Humidity : DisplayMode::Temperature;
  } else {
    Serial.print("Received unknown command: ");
    Serial.print(command);
    Serial.println();
  }
}

void readIR()
{
  if (IrReceiver.decode())
  {
    handleCommand(IrReceiver.decodedIRData.command);
    IrReceiver.resume();
  }
}

void check()
{
  readSensor();
  handleHeat();
}

void setupMemory() // RUN ONLY ONCE THE FIRST TIME CODE IS DEPLOYED
{
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up");
  // setupMemory(); RUN ONLY ONCE THE FIRST TIME CODE IS DEPLOYED
  setupSensor();
  setupHeater();
  setupIR();
  setupDisplay();
  t.every(CHECK_INTERVAL, check);
  t.every(IR_READ_INTERVAL, readIR);
}

void loop()
{
  writeDisplay();
  t.update();
}