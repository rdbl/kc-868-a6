#include <ArduinoJson.h>
#include <ArduinoYaml.h>
#include <SPIFFS.h>
#include "LoadConfig.h"

Config config;



// ----------------------------------------------------
// Fonction générique pour extraire une entité (device, actuators, sensors)
// ----------------------------------------------------
void extractEntity(JsonObject json, Entity &entity) {
    entity.id = json["id"] | "";
    entity.name = json["header"]["name"] | "";
    entity.model = json["header"]["model"] | "";
    entity.description = json["header"]["description"] | "";
    entity.pin = json["header"]["pin"] | NULL;
    entity.room = json["header"]["room"] | "";

    JsonObject datas = json["datas"].as<JsonObject>();

    // Convertir l'objet datas en JSON string pour stockage dynamique
    JsonDocument tempDoc;
    tempDoc.set(datas);
    serializeJson(tempDoc, entity.datasJson);
}

// ----------------------------------------------------
// Fonction loadConfig() optimisée
// ----------------------------------------------------
void loadConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Erreur lors du montage de SPIFFS");
        return;
    }

    File configFile = SPIFFS.open("/configuration.yaml", "r");
    if (!configFile) {
        Serial.println("Impossible d'ouvrir configuration.yaml");
        return;
    }

  // Lecture du contenu du fichier
    String yamlContent = configFile.readString();
    configFile.close();

    // Conversion du YAML en JSON via YAMLDuino
    YAMLNode yamlnode = YAMLNode::loadString(yamlContent.c_str());
    String jsonContent;
    serializeYml(yamlnode.getDocument(), jsonContent, OUTPUT_JSON);

    // Affichage pour débogage
    // Serial.println("JSON généré :");
    // Serial.println(jsonContent);

    // Désérialisation du JSON 
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        Serial.print("Erreur lors de la conversion YAML en JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Extraction des paramètres WiFi
    config.wifiSSID = doc["wifi"]["ssid"] | "defaultSSID";
    config.wifiPassword = doc["wifi"]["password"] | "defaultPassword";

    // Extraction des paramètres MQTT
    config.mqttHost = doc["mqtt"]["host"] | "defaultHost";
    config.mqttPort = doc["mqtt"]["port"] | 1883;
    config.mqttUser = doc["mqtt"]["user"] | "defaultUser";
    config.mqttPassword = doc["mqtt"]["password"] | "defaultMqttPassword";
    config.mqttTopic = doc["mqtt"]["topic"] | "defaultTopic";

    // Extraction du device principal
    extractEntity(doc["device"], config.device);

    // Extraction des actuators
    config.actuatorsCount = 0;
    JsonArray actuators = doc["device"]["actuators"].as<JsonArray>();
    for (JsonObject actuator : actuators) {
        if (config.actuatorsCount >= MAX_ACTUATORS) break;
        extractEntity(actuator, config.actuators[config.actuatorsCount]);
        config.actuatorsCount++;
    }

    // Extraction des sensors
    config.sensorsCount = 0;
    JsonArray sensors = doc["device"]["sensors"].as<JsonArray>();
    for (JsonObject sensor : sensors) {
        if (config.sensorsCount >= MAX_SENSORS) break;
        extractEntity(sensor, config.sensors[config.sensorsCount]);
        config.sensorsCount++;
    }

    Serial.println("Configuration chargée avec succès !");
}




// ----------------------------------------------------
// Fonction pour afficher la configuration
// ----------------------------------------------------
void printConfig(Config config) {
    Serial.println("=== CONFIGURATION ===");

    Serial.println("[WiFi]");
    Serial.println("SSID: " + config.wifiSSID);

    Serial.println("[MQTT]");
    Serial.println("Host: " + config.mqttHost);
    Serial.println("Port: " + String(config.mqttPort));
    Serial.println("User: " + config.mqttUser);
    Serial.println("Topic: " + config.mqttTopic);

    Serial.println("[Device]");
    Serial.println("ID: " + config.device.id);
    Serial.println("Name: " + config.device.name);
    Serial.println("Model: " + config.device.model);
    Serial.println("Description: " + config.device.description);

    Serial.println("[Actuators]");
    for (int i = 0; i < config.actuatorsCount; i++) {
        Serial.println(" - ID: " + config.actuators[i].id);
        Serial.println("   Name: " + config.actuators[i].name);
        Serial.println("   Model: " + config.actuators[i].model);
        Serial.println("   Description: " + config.actuators[i].description);
        Serial.println("   Pin: " + String(config.actuators[i].pin));
        Serial.println("   Datas JSON: " + config.actuators[i].datasJson);
    }

    Serial.println("[Sensors]");
    for (int i = 0; i < config.sensorsCount; i++) {
        Serial.println(" - ID: " + config.sensors[i].id);
        Serial.println("   Name: " + config.sensors[i].name);
        Serial.println("   Model: " + config.sensors[i].model);
        Serial.println("   Description: " + config.sensors[i].description);
        Serial.println("   Pin: " + String(config.sensors[i].pin));
        Serial.println("   Datas JSON: " + config.sensors[i].datasJson);
    }
}