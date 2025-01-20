/************************************
 *           INCLUSIONS
 ************************************/
#include <WiFiManager.h> // Bibliothèque pour WiFiManager
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include <U8g2lib.h>
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
PCF8574 pcf8574_I1(&I2Cone, 0x22, 4, 15);
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
int countdown = 0;
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
void displayInfos(unsigned long currentTime, float temp1, float temp2, float tempDifference, bool relayStates[], int countdown)
{
  // Affichage sur le port série pour debug
  Serial.print("Sonde 1: ");
  Serial.print(temp1);
  Serial.println(" °C [Bleu]");

  Serial.print("Sonde 2: ");
  Serial.print(temp2);
  Serial.println(" °C [Blanc]");

  Serial.print("Différentiel: ");
  Serial.print(tempDifference);
  Serial.println(" °C");

  Serial.print("Relais 1: ");
  Serial.println(relayStates[0] ? "ON" : "OFF");

  Serial.print("Temporisation: ");
  Serial.print(countdown);
  Serial.println(" s");

  // Alternance du point clignotant
  isDotVisible = !isDotVisible;

  // Affichage sur l'écran OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Affichage réorganisé
  int y = 10; // Ligne de départ
  char tempDisplay1[32];
  snprintf(tempDisplay1, sizeof(tempDisplay1), "S1: %.2f C [%s]", temp1,
           (temp1 != DEVICE_DISCONNECTED_C) ? "Bleu" : "Erreur");
  u8g2.drawStr(0, y, tempDisplay1);

  y = 22;
  char tempDisplay2[32];
  snprintf(tempDisplay2, sizeof(tempDisplay2), "S2: %.2f C [%s]", temp2,
           (temp2 != DEVICE_DISCONNECTED_C) ? "Blanc" : "Erreur");
  u8g2.drawStr(0, y, tempDisplay2);

  y = 34;
  char diffDisplay[32];
  snprintf(diffDisplay, sizeof(diffDisplay), "Diff: %.2f C", tempDifference);
  u8g2.drawStr(0, y, diffDisplay);

  y = 46;
  char relayDisplay[32];
  snprintf(relayDisplay, sizeof(relayDisplay), "Relais 1: %s", relayStates[0] ? "ON" : "OFF");
  u8g2.drawStr(0, y, relayDisplay);

  if (isCountdownActive)
  {
    y = 58;
    char countdownDisplay[32];
    snprintf(countdownDisplay, sizeof(countdownDisplay), "Tempo: %lus", countdown);
    u8g2.drawStr(0, y, countdownDisplay);
  }

  // Affichage du point clignotant
  if (isDotVisible)
  {
    u8g2.drawStr(120, 58, "."); // Point à droite de la dernière ligne
  }

  u8g2.sendBuffer();
}

/************************************
 *           SETUP
 ************************************/
void setup()
{
  // Initialisation du Serial pour debug
  Serial.begin(9600);
  delay(1000);

  // -- Étape 1 : WiFi Manager --
  // Création de l'objet WiFiManager
  WiFiManager wifiManager;

  // Si tu veux nettoyer l'ancienne config, décommente la ligne ci-dessous :
  // wifiManager.resetSettings();

  // Lance la config automatique :
  //  - Si l'ESP32 n'a pas de réseau sauvegardé, il crée un AP nommé "AutoConnectAP"
  //  - Sinon, il tente de se connecter aux identifiants déjà enregistrés
  if (!wifiManager.autoConnect("AutoConnectAP", "12345678"))
  {
    Serial.println("Failed to connect, rebooting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected successfully!");
  Serial.println(WiFi.localIP());

  // -- Étape 2 : Config OTA --
  setupOTA();

  // Init MQTT, avec ton broker, user/pass
  initMqtt("homeassistant.local", 1883, "romain", "2121Rom1");

  // -- Étape 3 : Initialisation OLED --
  u8g2.begin();

  // -- Étape 4 : Initialisation des sondes DS18B20 --
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

  // Petit message pour dire que le setup est fini
  Serial.println("Setup completed!");
}

/************************************
 *           LOOP
 ************************************/
void loop()
{
  // Gestion de l'OTA : à appeler en continu pour permettre l'update
  ArduinoOTA.handle();

  handleMqtt(); // Garde la connexion MQTT

  // Récupération des températures des deux sondes
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();

  float temp1 = sensors1.getTempCByIndex(0);
  float temp2 = sensors2.getTempCByIndex(0);

  // Calcul du différentiel
  float tempDifference = abs(temp1 - temp2);

  // Gestion de l'état du relais avec temporisation
  unsigned long currentTime = millis();

  // S'assure que le relais soit sur OFF avant toute logique
  pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);

  // Changement d'état si la temporisation n’est pas active
  if (!isCountdownActive)
  {
    // Si le différentiel est inférieur au seuil et le relais est actif (LOW)
    if (tempDifference < differentialThreshold && relayStates[0])
    {
      bool writeSuccess = pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
      if (writeSuccess)
      {
        Serial.println("Successfully set relay 0 to HIGH (OFF).");
        relayStates[0] = false;
        lastRelayChangeTime = currentTime;
        isCountdownActive = true;
      }
      else
      {
        Serial.println("[ERROR] Failed to set relay 0 to HIGH (OFF).");
        // Affiche une erreur sur l'écran OLED
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 58, "Erreur relais 1");
        u8g2.sendBuffer();
      }
    }
    // Si le différentiel est supérieur ou égal au seuil et le relais est inactif (HIGH)
    else if (tempDifference >= differentialThreshold && !relayStates[0])
    {
      bool writeSuccess = pcf8574_R1.digitalWrite(RELAY1_PIN, LOW);
      if (writeSuccess)
      {
        Serial.println("Successfully set relay 0 to LOW (ON).");
        relayStates[0] = true;
        lastRelayChangeTime = currentTime;
        isCountdownActive = true;
      }
      else
      {
        Serial.println("[ERROR] Failed to set relay 0 to LOW (ON).");
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 58, "Erreur relais 1");
        u8g2.sendBuffer();
      }
    }
  }

  // Mise à jour de la temporisation
  if (isCountdownActive && (currentTime - lastRelayChangeTime >= relayDelay))
  {
    isCountdownActive = false;
  }

  if (isCountdownActive)
  {
    countdown = (relayDelay - (currentTime - lastRelayChangeTime)) / 1000;
  }
  else
  {
    countdown = 0;
  }

  // Affichage des données sur l'écran OLED
  displayInfos(currentTime, temp1, temp2, tempDifference, relayStates, countdown);

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
    sensor1Data["timestamp"] = currentTime;

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
    sensor1Data["timestamp"] = currentTime;

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

  // Petite pause
  delay(1000);
}
