#include <Arduino.h>
#include <SevSeg.h>
#include <DHT.h>
#include <EEPROM.h>
#include <IRremote.h>
#include <Timer.h>
#include <EasyBuzzer.h>
#include <math.h>

// Control layout
// 69  70  71
// 68  64  67
// 7   21  9
// 22  25  13
// 0   24  94
// 8   28  90
// 66  82  74
#define IR_PIN 8
#define IR_READ_INTERVAL 500
#define IR_BLANK_CODE 4294967295 // ???
#define IR_TOGGLE_MODE_COMMAND 70
#define IR_TOGGLE_TURN_OFF_ALARM_COMMAND 71
#define IR_TOGGLE_LIGHT_COMMAND 69

#define DHT_TYPE DHT22
#define DHT_PIN 9

// Top Row:    1 A F  2 3 B
// Bottom Row: E D DP C G 4
#define DISPLAY_NUM_DIGITS 4
#define DISPLAY_DIGIT_PINS \
  {                        \
    2, 5, 6, A5            \
  }
#define DISPLAY_SEGMENTS_PINS   \
  {                             \
    3, 7, A3, A1, A0, 4, A4, A2 \
  }
#define DISPLAY_COMMON_TYPE COMMON_CATHODE
#define DISPLAY_BRIGHTNESS 10
#define DISPLAY_MODE_TEMPERATURE 1

#define LIGHT_SWITCH_PIN 10
#define HEAT_SWITCH_PIN 11
#define MINIMUM_TEMPERATURE 36.9f
#define MAXIMUM_TEMPERATURE 37.0f
#define CHECK_INTERVAL 10000

#define ALARM_PIN 12
#define ALARM_FREQUENCY 1000
#define ALARM_DURATION 100
#define ALARM_COOLDOWN 3600000
#define HIGH_TEMPERATURE_ALARM 38.0f
#define LOW_TEMPERATURE_ALARM 36.0f
#define HIGH_HUMIDITY_ALARM 80
#define LOW_HUMIDITY_ALARM 58

enum DisplayMode
{
  Temperature,
  Humidity
};

SevSeg sevseg; // Instantiate a seven segment object
Timer t;
Timer alarmCooldownTimer;

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0.0f;
float humidity = 0.0f;
bool displayMode = DisplayMode::Temperature;

bool tempDisplayEnabled = false;
char *tempDisplayString;

uint8_t alarm;
bool alarmOn = false;
bool alarmCooldown = false;

bool lightOn = true;

void tempDisplayOff()
{
  tempDisplayEnabled = false;
}

void tempDisplay(char *string)
{
  tempDisplayEnabled = true;
  tempDisplayString = string;
  t.after(5000, tempDisplayOff);
}

void setupBuzzer()
{
  EasyBuzzer.setPin(ALARM_PIN);
}

void setupIR()
{
  IrReceiver.begin(IR_PIN, false);
}

void setupHeater()
{
  pinMode(HEAT_SWITCH_PIN, OUTPUT);
  digitalWrite(HEAT_SWITCH_PIN, HIGH);
  pinMode(LIGHT_SWITCH_PIN, OUTPUT);
  digitalWrite(LIGHT_SWITCH_PIN, LOW);
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

void displayTemperatureHumidity()
{
  char buf[5];
  if (displayMode == DisplayMode::Temperature)
  {
    String(temperature, 2).toCharArray(buf, 10);
    buf[4] = 'C';
  }
  else if (displayMode == DisplayMode::Humidity)
  {
    String(humidity, 2).toCharArray(buf, 10);
    buf[4] = 'P';
  }
  sevseg.setChars(buf);
}

void writeDisplay()
{
  if (tempDisplayEnabled)
  {
    sevseg.setChars(tempDisplayString);
  }
  else
  {
    displayTemperatureHumidity();
  }

  sevseg.refreshDisplay();
}

void readSensor()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

void turnOnLight()
{
  digitalWrite(LIGHT_SWITCH_PIN, LOW);
}

void turnOffLight()
{
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
}

void turnOnHeater()
{
  digitalWrite(HEAT_SWITCH_PIN, LOW);
}

void turnOffHeater()
{
  digitalWrite(HEAT_SWITCH_PIN, HIGH);
}

void handleHeat()
{
  if (temperature <= MINIMUM_TEMPERATURE)
  {
    turnOnHeater();
  }
  else if (temperature > MAXIMUM_TEMPERATURE)
  {
    turnOffHeater();
  }
}

void triggerAlarm()
{
  if (!alarmOn && !alarmCooldown)
  {
    alarmOn = true;
    alarm = t.every(5000, []()
                    { EasyBuzzer.beep(ALARM_FREQUENCY, 1); });
  }
}

void handleAlarm()
{
  if (temperature < LOW_TEMPERATURE_ALARM || temperature > HIGH_TEMPERATURE_ALARM)
  {
    triggerAlarm();
  }
  else if (humidity < LOW_HUMIDITY_ALARM || humidity > HIGH_HUMIDITY_ALARM)
  {
    triggerAlarm();
  }
  else if (alarmOn)
  {
    t.stop(alarm);
  }
}

void handleCommand(unsigned long command)
{
  if (command == IR_BLANK_CODE)
    return;

  if (command == IR_TOGGLE_MODE_COMMAND)
  {
    displayMode = displayMode == DisplayMode::Temperature ? DisplayMode::Humidity : DisplayMode::Temperature;
  }
  else if (command == IR_TOGGLE_TURN_OFF_ALARM_COMMAND)
  {
    alarmCooldown = true;
    alarmOn = false;
    t.stop(alarm);
    alarmCooldownTimer.after(ALARM_COOLDOWN, []()
                             { alarmCooldown = false; });
  }
  else if (command == IR_TOGGLE_LIGHT_COMMAND)
  {
    lightOn = !lightOn;
    if (lightOn)
    {
      turnOnLight();
    }
    else
    {
      turnOffLight();
    }
  }
  else
  {
    // Serial.print("Received unknown command: ");
    // Serial.print(command);
    // Serial.println();
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
  handleAlarm();
}

void setup()
{
  // Serial.begin(9600);
  // Serial.println("Starting up");
  setupBuzzer();
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
  EasyBuzzer.update();
  t.update();
  alarmCooldownTimer.update();
}