/************************************
 *           INCLUSIONS
 ************************************/
#include "LoadConfig.h"
#include "HardwareManager.h"
#include "Display_connection.h"
#include "wifi_connection.h"
#include "OTA_connection.h"
#include "mqtt_connection.h"

// (À terme, vous pourrez ajouter d'autres managers, par exemple MQTTManager, DeviceManager, etc.)

/************************************
 *          VARIABLES GLOBALES
 ************************************/
DisplayConnection displayConnection;

// Variables de contrôle (pour la logique applicative, à extraire si nécessaire dans des managers spécifiques)
bool isCountdownActive = false;
bool isAutoMode = false;
unsigned long lastRelayChangeTime = 0;
String relayCommand = "on"; // Commande relais initiale
float differentialStartThreshold = 9.0;
float differentialStopThreshold = 3.0;
int relayDelay = 30000;

// Variable globale pour la construction du JSON à publier
JsonDocument rliot;

// Déclaration globale d'un HardwareManager
HardwareManager hwManager;

// Pour MQTT, vous utiliserez ultérieurement un MQTTManager qui se chargera de centraliser la communication

/************************************
 *          FONCTIONS
 ************************************/

// Logique automatique de la pompe, qui utilisera les objets matériels via HardwareManager
void handleAutomaticMode(float tempDifference) {
  unsigned long currentTime = millis();

  if (!isCountdownActive) {
    // Exemple : utiliser HardwareManager pour commander le relais
    // On suppose que HardwareManager expose une méthode pour commander le relais de la pompe
    if (tempDifference >= differentialStartThreshold && !hwManager.getPCF8574()->digitalRead(P0)) {
      hwManager.getPCF8574()->digitalWrite( P0 , HIGH);
      isAutoMode = true;  // Exemple d'utilisation
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe activée.");
      delay(1000);
    }
    else if (tempDifference < differentialStopThreshold && hwManager.getPCF8574()->digitalRead(P0)) {
      hwManager.getPCF8574()->digitalWrite( P0 , LOW);
      lastRelayChangeTime = currentTime;
      isCountdownActive = true;
      Serial.println("[AUTO] Pompe désactivée.");
      delay(1000);
    }
  }
  if (isCountdownActive && (currentTime - lastRelayChangeTime >= relayDelay)) {
    isCountdownActive = false;
    Serial.println("[AUTO] Temporisation terminée.");
  }
}

// Callback MQTT (à refactoriser ultérieurement dans un MQTTManager)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("[MQTT] Received relay command:");
  Serial.println(message);
  
  // Exemple de mise à jour du JSON à publier
  if (String(topic) == "Chauffage/bouilleur/homeassistant/actuators/0/params/state") {
    rliot["actuators"][0]["params"]["state"] = message;
    relayCommand = message;
    publishAllData(config.mqtt.topic, rliot);
  }
  // Autres conditions pour mode, diff_start, etc.
}

// Mise à jour du broker MQTT (à intégrer dans un futur MQTTManager)
void updateMqttBroker(float temp1, float temp2, float tempDifference) {
  rliot["id"] = config.device.id; // Utilisation de l'id défini dans la config

  JsonObject header = rliot["header"].to<JsonObject>();
  header["model"] = config.device.header.model;
  header["context"] = "chaufferie";

  JsonObject extra = rliot["extra"].to<JsonObject>();
  extra["temp_diff"] = tempDifference;
  extra["IsCountdownActive"] = isCountdownActive;
  extra["tempo"] = ((float)relayDelay - (millis() - lastRelayChangeTime)) / 1000.0 * isCountdownActive;

  // Mise à jour de la partie sensors
  JsonArray sensors = rliot["sensors"].to<JsonArray>();
  {
    JsonObject sensor1 = sensors.add<JsonObject>();
    sensor1["id"] = config.sensors[0].id;
    JsonObject sensor1Header = sensor1["header"].to<JsonObject>();
    sensor1Header["name"] = config.sensors[0].header.name;
    sensor1Header["model"] = config.sensors[0].header.model;
    JsonObject sensor1Data = sensor1["data"].to<JsonObject>();
    sensor1Data["value"] = temp1;
    sensor1["error"] = nullptr;
  }
  {
    JsonObject sensor2 = sensors.add<JsonObject>();
    sensor2["id"] = config.sensors[1].id;
    JsonObject sensor2Header = sensor2["header"].to<JsonObject>();
    sensor2Header["name"] = config.sensors[1].header.name;
    sensor2Header["model"] = config.sensors[1].header.model;
    JsonObject sensor2Data = sensor2["data"].to<JsonObject>();
    sensor2Data["value"] = temp2;
    sensor2["error"] = nullptr;
  }

  // Mise à jour des actuators
  JsonArray actuators = rliot["actuators"].to<JsonArray>();
  {
    JsonObject actuator1 = actuators.add<JsonObject>();
    actuator1["id"] = config.actuators[0].id;
    JsonObject actuator1Header = actuator1["header"].to<JsonObject>();
    actuator1Header["name"] = config.actuators[0].header.name;
    actuator1Header["model"] = config.actuators[0].header.model;
    JsonObject actuator1data = actuator1["params"].to<JsonObject>();
    actuator1data["state"] = relayCommand;
    actuator1data["mode"] = isAutoMode ? "AUTO" : "MANUAL";
    actuator1data["diff_start"] = differentialStartThreshold;
    actuator1data["diff_stop"] = differentialStopThreshold;
    actuator1data["relay_delay"] = relayDelay / 1000;
  }

  publishAllData(config.mqtt.topic, rliot);
}

/************************************
 *             SETUP
 ************************************/
void setup() {
  Serial.begin(9600);

  // Charger la configuration depuis le YAML
  loadConfig();
  printConfig(config);

  // Initialisation du hardware via HardwareManager (sensors, PCF8574, etc.)
  hwManager.begin();

  // Initialisation de l'affichage
  displayConnection.begin();

  // Initialiser le WiFi
  initializeWiFi(config.wifi.ssid, config.wifi.password);

  // Initialisation OTA
  setupOTA();

  // Initialisation MQTT (à refactoriser ultérieurement dans un MQTTManager)
  setMqttCallback(mqttCallback);
  initMqtt(config.mqtt.host, config.mqtt.port, config.mqtt.user, config.mqtt.password);

  Serial.println("Setup completed!");
}

/************************************
 *             LOOP
 ************************************/
void loop() {
  ArduinoOTA.handle();
  handleMqtt();

  // Mise à jour des capteurs via HardwareManager (gestion dynamique des sensors)
  // Par exemple, pour le premier capteur :
  hwManager.getSensor(0)->requestTemperatures();
  hwManager.getSensor(1)->requestTemperatures();
  delay(750);

  float temp1 = hwManager.getSensor(0)->getTempCByIndex(0);
  float temp2 = hwManager.getSensor(1)->getTempCByIndex(0);
  float tempDifference = temp1 - temp2;

  // Logique de contrôle de la pompe (la commande matérielle ici devra être adaptée pour utiliser le HardwareManager)
  // Par exemple, utiliser pcf8574 via HardwareManager
  if (isAutoMode) {
    handleAutomaticMode(tempDifference);
  }
  else {
    if (relayCommand == "on") {
      hwManager.getPCF8574()->digitalWrite( P0 , HIGH);
      // Mise à jour de l'état
    } else {
      hwManager.getPCF8574()->digitalWrite( P0 , LOW);
    }
  }

  updateMqttBroker(temp1, temp2, tempDifference);

  // Mise à jour de l'affichage
  // Vous pouvez appeler displayConnection.displayInfos() en lui passant les valeurs mesurées et les états

  // Debug
  Serial.printf("Temp S1: %.2f°C | Temp S2: %.2f°C | Diff: %.2f°C\n", temp1, temp2, tempDifference);
  delay(1000);
}
