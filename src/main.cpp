/************************************
 *           INCLUSIONS
 ************************************/
#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include <U8G2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PCF8574.h"

// Inclus le header MQTT
#include "mqtt_connection.h"

/************************************
 *         CONFIGURATION OLED
 ************************************/
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R2,      // Rotation
    15,           // clock
    4,            // data
    U8X8_PIN_NONE // reset
);

/************************************
 *   CONFIGURATION DES 1-Wire
 ************************************/
#define ONE_WIRE_BUS1 32 // GPIO pour la première sonde
#define ONE_WIRE_BUS2 33 // GPIO pour la seconde sonde

OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

/************************************
 *   CONFIGURATION PCF8574
 ************************************/
TwoWire I2Cone = TwoWire(0);
PCF8574 pcf8574_R1(&I2Cone, 0x24, 4, 15);

#define RELAY1_PIN P0 // Relais 1 contrôlé par la broche P0 du PCF8574
#define NUM_RELAYS 6  // Nombre de relais

/************************************
 *   VARIABLES & CONSTANTES
 ************************************/
bool isDotVisible = false;
float differentialThreshold = 2.0;
unsigned long lastRelayChangeTime = 0;
unsigned long relayDelay = 60000; // Temporisation de 1 minute (en ms)
bool isCountdownActive = false;
bool relayStates[NUM_RELAYS] = {false, false, false, false, false, false};

const char *myTopic = "Chauffage/bouilleur"; // Ton topic MQTT que tu veux utiliser

/************************************
 *       FONCTION : setupOTA
 ************************************/
void setupOTA()
{
  // Nom d’hôte du module (visible dans l’IDE Arduino lors de l’OTA)
  ArduinoOTA.setHostname("regulation-bouilleur");

  // Callbacks pour le debug
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { 
        type = "filesystem";
      }
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress * 100) / total); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)  Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)    Serial.println("End Failed"); });

  // Initialisation de l’OTA
  ArduinoOTA.begin();
  Serial.println("OTA Ready - Connecté au WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/************************************
 *  FONCTION : displayInfos
 ************************************/
void displayInfos(float temp1, float temp2, float tempDifference, bool relayState, int countdown)
{
  if(!isCountdownActive) countdown = 0;
  // Debug sur le port série
  Serial.printf("Temp S1: %.2f°C | Temp S2: %.2f°C | Diff: %.2f°C\n", temp1, temp2, tempDifference);
  Serial.printf("Relay State: %s | Countdown: %d s\n", relayState ? "ON" : "OFF", countdown);

  // Affichage OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "S1: %.2f°C", temp1);
  u8g2.drawStr(0, 10, buffer);

  snprintf(buffer, sizeof(buffer), "S2: %.2f°C", temp2);
  u8g2.drawStr(0, 22, buffer);

  snprintf(buffer, sizeof(buffer), "Diff: %.2f°C", tempDifference);
  u8g2.drawStr(0, 34, buffer);

  snprintf(buffer, sizeof(buffer), "Relay: %s", relayState ? "ON" : "OFF");
  u8g2.drawStr(0, 46, buffer);

  if (isCountdownActive)
  {
    snprintf(buffer, sizeof(buffer), "Tempo: %d s", countdown);
    u8g2.drawStr(0, 58, buffer);
  }

  // Point clignotant
  if (isDotVisible)
  {
    u8g2.drawStr(120, 58, ".");
  }
  isDotVisible = !isDotVisible;

  u8g2.sendBuffer();
}

/************************************
 *  LOGIQUE AUTOMATIQUE DE LA POMPE
 ************************************/
void handleAutomaticMode(float tempDifference)
{
  unsigned long currentTime = millis();

  // Si la temporisation est terminée, on évalue l'état
  if (!isCountdownActive)
  {
    if (tempDifference >= differentialThreshold && !relayStates[0])
    {
      // Activer la pompe
      pcf8574_R1.digitalWrite(RELAY1_PIN, LOW);
      relayStates[0] = true;
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe activée.");
    }
    else if (tempDifference < differentialThreshold && relayStates[0])
    {
      // Désactiver la pompe
      pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
      relayStates[0] = false;
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe désactivée.");
    }
  }

  // Gestion de la temporisation
  if (isCountdownActive && (currentTime - lastRelayChangeTime >= relayDelay))
  {
    isCountdownActive = false;
    Serial.println("[AUTO] Temporisation terminée.");
  }
}

/************************************
 *  MISE À JOUR DU BROKER MQTT
 ************************************/
void updateMqttBroker(float temp1, float temp2)
{
 // Construire le JSON
  JsonDocument rliot;
  rliot["id"] = "device_001";

  JsonObject header = rliot["header"].to<JsonObject>();
  header["model"] = "kc868-A6";
  header["context"] = "chaufferie";

  // --- Partie sensors[] ---
  JsonArray sensors = rliot["sensors"].to<JsonArray>();
  // Sensor 1: Temperature
  {
    JsonObject sensor1 = sensors.add<JsonObject>();
    sensor1["id"] = "szehdjksdfj6871";
    JsonObject sensor1Header = sensor1["header"].to<JsonObject>();
    sensor1Header["name"] = "sonde 1 bleue";
    sensor1Header["model"] = "DS18B20";
    // On ne reproduit pas ici toutes les pins, doc, etc. par souci de lisibilité
    // Mais tu peux les ajouter comme dans ton JSON de référence

    sensor1["status"] = "OK"; // ou un statut dynamique

    JsonObject sensor1Data = sensor1["data"].to<JsonObject>();
    sensor1Data["value"] = temp1;

    sensor1["error"] = nullptr; // ou un message si tu as un souci
  }

  {
    JsonObject sensor1 = sensors.add<JsonObject>();
    sensor1["id"] = "szehdjksdfj6882";
    JsonObject sensor1Header = sensor1["header"].to<JsonObject>();
    sensor1Header["name"] = "sonde 2 blanche";
    sensor1Header["model"] = "DS18B20";
    // On ne reproduit pas ici toutes les pins, doc, etc. par souci de lisibilité
    // Mais tu peux les ajouter comme dans ton JSON de référence

    sensor1["status"] = "OK"; // ou un statut dynamique

    JsonObject sensor1Data = sensor1["data"].to<JsonObject>();
    sensor1Data["value"] = temp2;

    sensor1["error"] = nullptr; // ou un message si tu as un souci
  }

  // --- Partie actuators[] ---
  JsonArray actuators = rliot["actuators"].to<JsonArray>();

  // Actuator 1: Relay (Lighting ou Door, à toi de voir)
  {
    JsonObject actuator1 = actuators.add<JsonObject>();
    actuator1["id"] = "relay_001";
    JsonObject actuator1Header = actuator1["header"].to<JsonObject>();
    actuator1Header["name"] = "pompe bouilleur";
    actuator1Header["model"] = "Relay 220v";
    JsonObject actuator1data = actuator1["data"].to<JsonObject>();
    actuator1data["state"] = relayStates[0] ? "on" : "off";
  }

  // Publier le JSON dans "myTopic"
  publishAllData(myTopic, rliot);
}

/************************************
 *           SETUP
 ************************************/
void setup()
{
  Serial.begin(9600);

  // WiFi
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("AutoConnectAP"))
  {
    Serial.println("Failed to connect, rebooting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("WiFi connected successfully!");
  Serial.println(WiFi.localIP());

  setupOTA();

  // -- Étape 3 : Initialisation OLED --
  u8g2.begin();

 // MQTT
  initMqtt("homeassistant.local", 1883, "romain", "2121Rom1");

  // Initialisation des capteurs et relais
  sensors1.begin();
  sensors2.begin();

  // -- Étape 5 : Initialisation PCF8574 / Relais --
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pcf8574_R1.pinMode(i, OUTPUT);
  }
  pcf8574_R1.begin();

  // Met tous les relais sur HIGH (OFF)
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pcf8574_R1.digitalWrite(i, HIGH);
  }

  // S'assure que le relais soit sur OFF avant toute logique
  pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);

  Serial.println("Setup completed!");
 
}

/************************************
 *           LOOP
 ************************************/
void loop()
{
  ArduinoOTA.handle();
  handleMqtt();

  // Lecture des températures
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();
  float temp1 = sensors1.getTempCByIndex(0);
  float temp2 = sensors2.getTempCByIndex(0);
  float tempDifference = abs(temp1 - temp2);

  // S'assure que le relais soit sur OFF avant toute logique
  pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);

  // Logique automatique
  handleAutomaticMode(tempDifference);

  // Mise à jour du broker MQTT
  updateMqttBroker(temp1, temp2);

  // Mise à jour de l'affichage
  displayInfos(temp1, temp2, tempDifference, relayStates[0], (relayDelay - (millis() - lastRelayChangeTime)) / 1000);

  delay(1000);
}
