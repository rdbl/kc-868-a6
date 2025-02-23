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
int countdown = 0;
bool isAutoMode = true;
unsigned long lastRelayChangeTime = 0;
String relayCommand = "on"; // Commande relais initiale
float differentialStartThreshold = 9.0;
float differentialStopThreshold = 3.0;
int relayDelay = 30000; // 30 secondes

// Variable globale pour la construction du JSON à publier
JsonDocument rliot;

// Déclaration globale d'un HardwareManager
HardwareManager hwManager;

// Pour MQTT, vous utiliserez ultérieurement un MQTTManager qui se chargera de centraliser la communication

/************************************
 *          FONCTIONS
 ************************************/

// Logique automatique de la pompe, qui utilisera les objets matériels via HardwareManager
void handleAutomaticMode(float differentialStartThreshold , float differentialStopThreshold , bool relayState , float tempDifference , unsigned long currentTime) {

    // Exemple : utiliser HardwareManager pour commander le relais
    // On suppose que HardwareManager expose une méthode pour commander le relais de la pompe

    // debug messages
    // Serial.printf("Differential start threshold: %.2f°C\n", differentialStartThreshold);
    // Serial.printf("Differential stop threshold: %.2f°C\n", differentialStopThreshold);
    // Serial.printf("Temp diff: %.2f°C\n", tempDifference);
    // Serial.printf("Relay state: %d\n", relayState);
    // Serial.printf("Current time: %lu\n", currentTime);
    // Serial.printf("Last relay change time: %lu\n", lastRelayChangeTime);


    if (tempDifference >= differentialStartThreshold && relayState == LOW && isCountdownActive == false) { 
      // si la différence de température est supérieure au seuil de démarrage et que le relay est LOW 
      // ( 0 = ouvert , diode allumée ) donc la pompe est ETEINTE parcequ'elle est branché sur le NO
      hwManager.getPCF8574()->digitalWrite( P0 , HIGH);
      lastRelayChangeTime = currentTime;
      Serial.println("[AUTO]-[main.cpp]- Pompe activée.");
      isCountdownActive = true;
      delay(1000);

    }
    else if (tempDifference < differentialStopThreshold &&  relayState == HIGH && isCountdownActive == false) { 
      // si la différence de température est inférieure au seuil d'arrêt et que le relay est HIGH 
      //( 1 = fermé , diode éteinte ) donc la pompe est ALLUMEE parcequ'elle est branché sur le NO
      hwManager.getPCF8574()->digitalWrite( P0 , LOW);
      lastRelayChangeTime = currentTime;
      Serial.println("[AUTO]-[main.cpp]- Pompe désactivée.");
      delay(1000);
      isCountdownActive = true;
    }{
      Serial.println("[AUTO]-[main.cpp]- Aucune action.");
    }
}

// Callback MQTT (à refactoriser ultérieurement dans un MQTTManager)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
    JsonDocument doc;
    deserializeJson(doc, message);
    Serial.println("[MQTT] Received relay command.");
    Serial.print("[MQTT] Payload: ");
    Serial.println(message);
    if (String(topic) == (config.mqtt.topic+"/homeassistant/actuators/0/params/state")) {
        rliot["actuators"][0]["params"]["state"] = message;
        relayCommand = message;
        publishAllData(config.mqtt.topic, rliot);
    } else if (String(topic) == (config.mqtt.topic+"/homeassistant/actuators/0/params/mode")) {
        rliot["actuators"][0]["params"]["mode"] = message == "AUTO" ? "AUTO" : "MANUAL"; ;
        isAutoMode = message == "AUTO" ? true : false;
        publishAllData(config.mqtt.topic, rliot);
    } else if (String(topic) == (config.mqtt.topic+"/homeassistant/actuators/0/params/diff_start")) {
        rliot["actuators"][0]["params"]["diff_start"] = message ;
        differentialStartThreshold = message.toFloat();
        publishAllData(config.mqtt.topic, rliot);
    } else if (String(topic) == (config.mqtt.topic+"/homeassistant/actuators/0/params/diff_stop")) {
        rliot["actuators"][0]["params"]["diff_stop"] = message ;
        differentialStopThreshold = message.toFloat();
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

  Serial.println("[DEBUG]-[main.cpp]- Setup completed!");
}

/************************************
 *             LOOP
 ************************************/
void loop() {
  ArduinoOTA.handle();
  handleMqtt();

    // intialisation du compteur de temps


  // Mise à jour des capteurs via HardwareManager (gestion dynamique des sensors)
  // Par exemple, pour le premier capteur :
  hwManager.getSensor(0)->requestTemperatures();
  hwManager.getSensor(1)->requestTemperatures();
  delay(750);
  // debug messages
  // Serial.println("requestTemperatures()");
  float temp1 = hwManager.getSensor(0)->getTempCByIndex(0);
  float temp2 = hwManager.getSensor(1)->getTempCByIndex(0);
  float tempDifference = temp1 - temp2;
  // debug messages
  Serial.printf("[DEBUG]-[main.cpp]- Temp S1: %.2f°C | Temp S2: %.2f°C | Diff: %.2f°C\n", temp1, temp2, tempDifference);
  // probleme de lecture des valeurs du relais
  // bool relayState = hwManager.getPCF8574()->digitalRead(P0);
  // [ 28461][E][Wire.cpp:513] requestFrom(): i2cRead returned Error 263


  bool relayState = hwManager.getPCF8574()->digitalRead(P0);


  // Logique de contrôle de la pompe (la commande matérielle ici devra être adaptée pour utiliser le HardwareManager)
  // Par exemple, utiliser pcf8574 via HardwareManager pour commander le relais

  unsigned long currentime = millis()/1000;
  if(isCountdownActive){
    Serial.print("relayDelay : ");
    Serial.print(relayDelay);
    countdown = relayDelay / 1000 - (currentime - lastRelayChangeTime);
    Serial.print(" | countdown : ");
    Serial.print(countdown);
    Serial.printf("Temporisation en cours: %d s\n", countdown);
    if (countdown <= 0) {
      isCountdownActive = false;
      Serial.println("Temporisation terminée.");
    }
  }else{
    countdown = 0;
  }

  if (isAutoMode) {
    Serial.println("[AUTO]-[main.cpp]- Automatic mode...");
    handleAutomaticMode(differentialStartThreshold, differentialStopThreshold, relayState , tempDifference, currentime );
  }
  else {
    if (relayCommand == "on") {
      Serial.println("comande recu ! Relay ON");
      hwManager.getPCF8574()->digitalWrite( 0 , HIGH);
      // Mise à jour de l'état
    } else {
      Serial.println("comande recu ! Relay OFF");
      hwManager.getPCF8574()->digitalWrite( 0 , LOW);
    }
  }

  updateMqttBroker(temp1, temp2, tempDifference);

  // Mise à jour de l'affichage
  // Vous pouvez appeler displayConnection.displayInfos() en lui passant les valeurs mesurées et les états
  Serial.println("[DISPLAY]-[main.cpp]- Updating display...");
  displayConnection.displayInfos(temp1, temp2, tempDifference , relayState, countdown, isCountdownActive, isAutoMode);
  Serial.println("[DISPLAY]-[main.cpp]- Display updated!");
  delay(1000);
}
