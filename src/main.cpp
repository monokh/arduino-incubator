#include <Arduino.h>

int LED_PIN = 5;

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up");
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  delay(100);
  analogWrite(LED_PIN, 255);
}