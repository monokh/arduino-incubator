#include <Arduino.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPIN 2
#define LEDPIN 13

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up");
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  dht.begin();
}

void loop()
{
  delay(1000);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" *C\t");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  if (temperature > 37.0f) {
    digitalWrite(LEDPIN, HIGH);
  } else {
    digitalWrite(LEDPIN, LOW);
  }
}