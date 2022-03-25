#include <Arduino.h>
#include <SDI12.h>
#include <MKRWAN.h>
#include "ap200.hpp"

constexpr uint16_t PORT_SERIE = 115200;
constexpr uint8_t DATA_PIN = 12;
constexpr uint8_t WAKE_UP_PIN = 6;
;
LoRaModem modem;
Ap2000Sensor sensor(DATA_PIN, 0);
const String appEui = "0000000000000000";
const String appKey = "EC6480D307B1C23E67BB53B9804E0C11";

void setup()
{

  Serial.begin(PORT_SERIE);
  sensor.SensorBegin();
  delay(5000);

  if (!modem.begin(EU868)) // Setup LoRa
  { 
    Serial.println("Echec du démarrage du module");
    while (1)
    {
    }
  }

  Serial.println(modem.version());
  Serial.println(modem.deviceEUI());
  int connected = modem.joinOTAA(appEui, appKey);

  if (!connected)
  {
    Serial.println("Un problème est survenu");
    while (1)
    {
    }
  }

  pinMode(WAKE_UP_PIN, OUTPUT);
}

void loop()
{

  digitalWrite(WAKE_UP_PIN, HIGH); // Wake up the black box
  delay(400000);                   // Data won't be ready and stable until 40s

  sensor.DispSensorInfos(Serial); // Debug test

  SensorMeasurements *sm = new SensorMeasurements; // Get measurments
  sensor.RequestMeasurementsAndWaitUntilReady();
  sensor.GetSensorMeasurements(sm, 16, false);
  Serial.println(sm->ph);
  Serial.println(sm->turbidity);

  float tmp = sm->temperature;
  float oxy = sm->dissolvedOxygenSaturation;
  float sal = sm->salinity;
  float tur = sm->turbidity;
  float ph = sm->ph;
  delete sm;

  Serial.println("temperature: " + String(tmp) + "°C"); // Debug output
  Serial.println("oxygene: " + String(oxy) + "%");
  Serial.println("salinite: " + String(sal) + "g/L");
  Serial.println("turbitité: " + String(tur) + "FNU");
  Serial.println("ph: " + String(ph));
  Serial.println("\n___________________________________\n");

  uint16_t tTmp = tmp * 100; // Data formatting for sending through LoRa
  uint16_t tOxy = oxy * 100;
  uint16_t tSal = sal * 100;
  uint16_t tTur = tur * 100;
  uint16_t tPh = ph * 100;

  byte payload[10]; // Payload construction
  payload[0] = highByte(tTmp);
  payload[1] = lowByte(tTmp);
  payload[2] = highByte(tOxy);
  payload[3] = lowByte(tOxy);
  payload[4] = highByte(tSal);
  payload[5] = lowByte(tSal);
  payload[6] = highByte(tTur);
  payload[7] = lowByte(tTur);
  payload[8] = highByte(tPh);
  payload[9] = lowByte(tPh);

  modem.beginPacket(); // Sending data via LoRa
  modem.write(payload, sizeof(payload));
  int err = modem.endPacket(true);
  if (err > 0)
  {
    Serial.println("Message bien envoyé");
    Serial.println();
  }
  else
  {
    Serial.println("Erreur lors de l'envoi du message");
  }

  digitalWrite(WAKE_UP_PIN, LOW);

  delay(30 * 60 * 1000); // Wait half an hour
}