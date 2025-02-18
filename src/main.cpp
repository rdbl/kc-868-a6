/************************************
 *           INCLUSIONS
 ************************************/


#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PCF8574.h"


#include "OTA_connection.h"
#include "wifi_connection.h"
#include "LoadConfig.h"
#include "Display_connection.h"
#include "mqtt_connection.h"


/************************************
 *   CONFIGURATION DES 1-Wire
 ************************************/
#define ONE_WIRE_BUS1 33 // GPIO pour la première sonde
#define ONE_WIRE_BUS2 25 // GPIO pour la seconde sonde

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
DisplayConnection displayConnection;

const bool SIMULATION_MODE = false; // Mode simulation pour les tests

bool isCountdownActive = false;
bool isAutoMode = false; // Mode manuel par défaut

unsigned long lastRelayChangeTime = 0;

bool relayStates[NUM_RELAYS] = {false, false, false, false, false, false};

String relayCommand = "on"; // Commande relais initiale
float differentialStartThreshold = 9; // Seuil de démarrage de la pompe
float differentialStopThreshold = 3; // Seuil d'arrêt de la pompe
int relayDelay = 30000; // Délai de temporisation de la pompe

 // Construire le JSON
JsonDocument rliot;




/************************************
 *  LOGIQUE AUTOMATIQUE DE LA POMPE
 ************************************/
void handleAutomaticMode(float tempDifference)
{
  unsigned long currentTime = millis();

  if (!isCountdownActive)
  {
    // Déclenchement de la pompe si la température dépasse le seuil de démarrage
    if (tempDifference >= differentialStartThreshold && !relayStates[0])
    {
      pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
      relayStates[0] = true;
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe activée.");
      delay(1000); // Stabilisation
    }
    // Arrêt de la pompe si la température redescend sous le seuil d'arrêt
    else if (tempDifference < differentialStopThreshold && relayStates[0])
    {
      pcf8574_R1.digitalWrite(RELAY1_PIN, LOW);
      relayStates[0] = false;
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe désactivée.");
      delay(1000); // Stabilisation
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
 *  Fonction d'échanges avec le broker mqtt
 ************************************/
// Callback MQTT pour traiter les messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "" ;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    JsonDocument doc;
    deserializeJson(doc, message);
    Serial.println("[MQTT] Received relay command.");
    Serial.print("[MQTT] Payload: ");
    Serial.println(message);

    if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/state") {
        rliot["actuators"][0]["params"]["state"] = message;
        relayCommand = message;
        publishAllData(config.mqttTopic, rliot);
    } else if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/mode") {
        rliot["actuators"][0]["params"]["mode"] = message == "AUTO" ? "AUTO" : "MANUAL";
        isAutoMode = message == "AUTO";
        publishAllData(config.mqttTopic, rliot);
    } else if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/diff_start") {
        differentialStartThreshold = message.toFloat();
        rliot["actuators"][0]["params"]["diff_start"] = differentialStartThreshold;
        publishAllData(config.mqttTopic, rliot);
    } else if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/diff_stop") {
        differentialStopThreshold = message.toFloat();
        rliot["actuators"][0]["params"]["diff_stop"] = differentialStopThreshold;
        publishAllData(config.mqttTopic, rliot);
    } else if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/relay_delay") {
        relayDelay = message.toInt() * 1000;
        rliot["actuators"][0]["params"]["relay_delay"] = relayDelay / 1000;
        publishAllData(config.mqttTopic, rliot);
    }
}


void updateMqttBroker(float temp1, float temp2, float tempDifference)
{
  rliot["id"] = "device_001";

  JsonObject header = rliot["header"].to<JsonObject>();
  header["model"] = "kc868-A6";
  header["context"] = "chaufferie";

  // --- Partie extra[] --- là où on met les données supplémentaires qui concernent le système

  JsonObject extra = rliot["extra"].to<JsonObject>();
  extra["temp_diff"] = tempDifference;
  extra["IsCountdownActive"] = isCountdownActive;
  extra["tempo"] = (relayDelay - (millis() - lastRelayChangeTime)) / 1000 * isCountdownActive ;
  

  // --- Partie sensors[] ---
  JsonArray sensors = rliot["sensors"].to<JsonArray>();
  {
    JsonObject sensor1 = sensors.add<JsonObject>();
    sensor1["id"] = "szehdjksdfj6871";
    // sensor1["status"] = "OK";
    JsonObject sensor1Header = sensor1["header"].to<JsonObject>();
    sensor1Header["name"] = "sonde 1 bleue";
    sensor1Header["model"] = "DS18B20";
    JsonObject sensor1Data = sensor1["data"].to<JsonObject>();
    sensor1Data["value"] = temp1;
    sensor1["error"] = nullptr;
  }

  {
    JsonObject sensor2 = sensors.add<JsonObject>();
    sensor2["id"] = "szehdjksdfj6882";
    // sensor2["status"] = "OK";
    JsonObject sensor2Header = sensor2["header"].to<JsonObject>();
    sensor2Header["name"] = "sonde 2 blanche";
    sensor2Header["model"] = "DS18B20";
    JsonObject sensor2Data = sensor2["data"].to<JsonObject>();
    sensor2Data["value"] = temp2;
    sensor2["error"] = nullptr;
  }

  // --- Partie actuators[] ---
  JsonArray actuators = rliot["actuators"].to<JsonArray>();
  {
    JsonObject actuator1 = actuators.add<JsonObject>();
    actuator1["id"] = "relay_001";
    JsonObject actuator1Header = actuator1["header"].to<JsonObject>();
    actuator1Header["name"] = "pompe bouilleur";
    actuator1Header["model"] = "Relay 220v";
    JsonObject actuator1data = actuator1["params"].to<JsonObject>();
    actuator1data["state"] = relayStates[0] ? "on" : "off";
    actuator1data["mode"] = isAutoMode ? "AUTO" : "MANUAL";
    actuator1data["diff_start"] = differentialStartThreshold;
    actuator1data["diff_stop"] = differentialStopThreshold;
    actuator1data["relay_delay"] = relayDelay / 1000 ;
  }

  // Publier le JSON dans "myTopic"
  publishAllData(config.mqttTopic, rliot);
}



/************************************
 *           SETUP
 ************************************/
void setup()
{
  Serial.begin(9600);


  // Charger la configuration
  loadConfig();
  printConfig(config);

  // Initialisation Display OLED
  displayConnection.begin();

  // Initialiser le WiFi
  initializeWiFi(config.wifiSSID, config.wifiPassword);

  // -- Étape 2 : Initialisation OTA --
  setupOTA();

 // MQTT
  setMqttCallback(mqttCallback); 
  initMqtt(config.mqttHost, config.mqttPort, config.mqttUser, config.mqttPassword);

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

  // S'assure que le relai 1 soit sur ON pour activer la pompe par défaut (sécurité)
  pcf8574_R1.digitalWrite(RELAY1_PIN, LOW);

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

  delay(750); // Temps de stabilisation pour éviter les lectures fausses

  float temp1 = sensors1.getTempCByIndex(0);
  float temp2 = sensors2.getTempCByIndex(0);

  if (SIMULATION_MODE == true) {
      if (SIMULATION_MODE) {
          Serial.println("[ERREUR] Sonde 1 ou 2 non connectée ! Simulation activée.");
          
          // Simulation réaliste avec random walk + bruit progressif
          static float baseTemp1 = 50.0, baseTemp2 = 35.0;
          static float noiseFactor = 2.0, amplitude = 10.0;

          float time = millis() / 10000.0; // Temps en secondes

          temp1 = baseTemp1 + sin(time) * amplitude + (random(-noiseFactor, noiseFactor + 1) / 2.0);
          temp2 = baseTemp2 + sin(time + 1.0) * amplitude + (random(-noiseFactor, noiseFactor + 1) / 2.0);

      } else if (temp1 == DEVICE_DISCONNECTED_C ) {
          Serial.println("[ERREUR] Sonde 1 déconnectée ! Sécurité activée.");
          pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
          relayStates[0] = false;
          return;
      } else if (temp2 == DEVICE_DISCONNECTED_C ) {
          Serial.println("[ERREUR] Sonde 2 déconnectée ! Sécurité activée.");
          pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
          relayStates[0] = false;
          return; 
      }
  }

  float tempDifference = temp1 - temp2;

  // S'assure que le relais soit sur OFF avant toute logique
  pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);

    if (isAutoMode) {
              // Mode automatique : logique de température
        handleAutomaticMode(tempDifference);

    } else {
              // Mode manuel : appliquer la commande du relais
        if (relayCommand == "on") {
            pcf8574_R1.digitalWrite(RELAY1_PIN, HIGH);
            relayStates[0] = true;
        } else {
            pcf8574_R1.digitalWrite(RELAY1_PIN, LOW);
            relayStates[0] = false;
        }
    }

  // Mise à jour du broker MQTT
  updateMqttBroker(temp1, temp2, tempDifference);

  int countdown = (relayDelay - (millis() - lastRelayChangeTime)) / 1000 * isCountdownActive;
  // Mise à jour de l'affichage
  displayConnection.displayInfos(
                      temp1, 
                      temp2, 
                      tempDifference, 
                      relayStates[0], 
                      countdown,
                      isCountdownActive, 
                      isAutoMode);
  
    // Debug sur le port série
    Serial.printf("Temp S1: %.2f°C | Temp S2: %.2f°C | Diff: %.2f°C\n", temp1, temp2, tempDifference);
    Serial.printf("Relay State: %s | Countdown: %d s\n", relayStates[0] ? "ON" : "OFF", countdown);


  delay(1000);
}
