#include <ArduinoJson.h>
#include <ArduinoYaml.h>
#include <SPIFFS.h>
#include "LoadConfig.h"

Config config;

// ----------------------------------------------------
// Fonction générique pour extraire une entité (device, displays, actuators, sensors)
// ----------------------------------------------------
void extractEntity(JsonObject json, Entity &entity) {
    // Extraction de l'ID
    entity.id = json["id"] | "";

    // Extraction du header
    JsonObject headerObj = json["header"].as<JsonObject>();
    if (!headerObj.isNull()) {
        entity.header.name = headerObj["name"] | "";
        entity.header.model = headerObj["model"] | "";
        entity.header.description = headerObj["description"] | "";
        entity.header.room = headerObj["room"] | "";
    } else {
        entity.header.name = "";
        entity.header.model = "";
        entity.header.description = "";
        entity.header.room = "";
    }

    // Extraction de la partie hardware (stockée sous forme de chaîne JSON)
    if (!json["hardware"].isNull()) {
        JsonDocument hwDoc;
        hwDoc.set(json["hardware"]);

        String hwJson;
        serializeJson(hwDoc, hwJson);
        entity.hardwareJson = hwJson;
    } else {
        entity.hardwareJson = "";
    }

    // Extraction de la partie data (stockée sous forme de chaîne JSON)
    if (!json["data"].isNull()) {
        JsonDocument dataDoc;
        dataDoc.set(json["data"]);
        String dataJson;
        serializeJson(dataDoc, dataJson);
        entity.dataJson = dataJson;
    } else {
        entity.dataJson = "";
    }
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

    // Debug
    Serial.println("YAML content:");
    Serial.println(yamlContent);

    // Conversion du YAML en JSON via ArduinoYaml
    YAMLNode yamlnode = YAMLNode::loadString(yamlContent.c_str());
    String jsonContent;
    serializeYml(yamlnode.getDocument(), jsonContent, OUTPUT_JSON);

    // Correction du défaut de serialisation des valeurs hexadécimales dans le JSON
    // rajout des "" autour des valeurs hexadécimales
    jsonContent = correctHexInJson(jsonContent);
    // // Debug
    // Serial.println("JSON content:");
    // Serial.println(jsonContent);

    // Désérialisation du JSON 
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        Serial.print("Erreur lors de la conversion YAML en JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Extraction du mode simulation
    config.simulationMode = doc["SIMULATION_MODE"] | false;

    // Extraction des paramètres WiFi
    config.wifi.ssid = doc["wifi"]["ssid"] | "defaultSSID";
    config.wifi.password = doc["wifi"]["password"] | "defaultPassword";

    // Extraction des paramètres MQTT
    config.mqtt.host = doc["mqtt"]["host"] | "defaultHost";
    config.mqtt.port = doc["mqtt"]["port"] | 1883;
    config.mqtt.user = doc["mqtt"]["user"] | "defaultUser";
    config.mqtt.password = doc["mqtt"]["password"] | "defaultMqttPassword";
    config.mqtt.topic = doc["mqtt"]["topic"] | "defaultTopic";

    // Extraction des paramètres OTA
    config.ota.enabled = doc["ota"]["enabled"] | false;
    config.ota.port = doc["ota"]["port"] | 3232;

    // Extraction du device principal
    extractEntity(doc["device"], config.device);

    // Extraction des displays
    config.displaysCount = 0;
    if (!doc["device"]["displays"].isNull()) {
        JsonArray displays = doc["device"]["displays"].as<JsonArray>();
        for (JsonObject display : displays) {
            if (config.displaysCount >= MAX_DISPLAYS) break;
            extractEntity(display, config.displays[config.displaysCount]);
            config.displaysCount++;
        }
    }

    // Extraction des actuators
    config.actuatorsCount = 0;
    if (!doc["device"]["actuators"].isNull()) {
        JsonArray actuators = doc["device"]["actuators"].as<JsonArray>();
        for (JsonObject actuator : actuators) {
            if (config.actuatorsCount >= MAX_ACTUATORS) break;
            extractEntity(actuator, config.actuators[config.actuatorsCount]);
            config.actuatorsCount++;
        }
    }

    // Extraction des sensors
    config.sensorsCount = 0;
    if (!doc["device"]["sensors"].isNull()) {
        JsonArray sensors = doc["device"]["sensors"].as<JsonArray>();
        for (JsonObject sensor : sensors) {
            if (config.sensorsCount >= MAX_SENSORS) break;
            extractEntity(sensor, config.sensors[config.sensorsCount]);
            config.sensorsCount++;
        }
    }

    Serial.println("Configuration chargée avec succès !");
}

// ----------------------------------------------------
// Fonction pour afficher la configuration
// ----------------------------------------------------
void printConfig(const Config &config) {
    Serial.println("=== CONFIGURATION ===");

    Serial.println("[WiFi]");
    Serial.println("SSID: " + config.wifi.ssid);
    Serial.println("Password: " + config.wifi.password);

    Serial.println("[MQTT]");
    Serial.println("Host: " + config.mqtt.host);
    Serial.println("Port: " + String(config.mqtt.port));
    Serial.println("User: " + config.mqtt.user);
    Serial.println("Topic: " + config.mqtt.topic);

    Serial.println("[OTA]");
    Serial.println("Enabled: " + String(config.ota.enabled));
    Serial.println("Port: " + String(config.ota.port));

    Serial.println("[Device]");
    Serial.println("ID: " + config.device.id);
    Serial.println("Name: " + config.device.header.name);
    Serial.println("Model: " + config.device.header.model);
    Serial.println("Description: " + config.device.header.description);
    Serial.println("Room: " + config.device.header.room);
    Serial.println("Hardware JSON: " + config.device.hardwareJson);
    Serial.println("Data JSON: " + config.device.dataJson);

    Serial.println("[Displays]");
    for (int i = 0; i < config.displaysCount; i++) {
        Serial.println(" - ID: " + config.displays[i].id);
        Serial.println("   Name: " + config.displays[i].header.name);
        Serial.println("   Model: " + config.displays[i].header.model);
        Serial.println("   Description: " + config.displays[i].header.description);
        Serial.println("   Hardware JSON: " + config.displays[i].hardwareJson);
        Serial.println("   Data JSON: " + config.displays[i].dataJson);
    }

    Serial.println("[Actuators]");
    for (int i = 0; i < config.actuatorsCount; i++) {
        Serial.println(" - ID: " + config.actuators[i].id);
        Serial.println("   Name: " + config.actuators[i].header.name);
        Serial.println("   Model: " + config.actuators[i].header.model);
        Serial.println("   Description: " + config.actuators[i].header.description);
        Serial.println("   Hardware JSON: " + config.actuators[i].hardwareJson);
        Serial.println("   Data JSON: " + config.actuators[i].dataJson);
    }

    Serial.println("[Sensors]");
    for (int i = 0; i < config.sensorsCount; i++) {
        Serial.println(" - ID: " + config.sensors[i].id);
        Serial.println("   Name: " + config.sensors[i].header.name);
        Serial.println("   Model: " + config.sensors[i].header.model);
        Serial.println("   Description: " + config.sensors[i].header.description);
        Serial.println("   Hardware JSON: " + config.sensors[i].hardwareJson);
        Serial.println("   Data JSON: " + config.sensors[i].dataJson);
    }
}

bool isHexDigit(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

String correctHexInJson(String content) {
  int searchIndex = 0;
  // Recherche la séquence ": 0x" dans le JSON
  while ((searchIndex = content.indexOf(": 0x", searchIndex)) != -1) {
    // Calculer l'indice de début du token hexadécimal
    // Exemple: dans ':"0x24', ": " occupe 0-1 et "0" commence à l'indice 2
    int tokenStart = searchIndex + 2; // devrait pointer sur '0'
    
    // Si le caractère juste après ": " est déjà une guillemet, on passe à la suite
    if (content.charAt(tokenStart) == '"') {
      searchIndex = tokenStart + 1;
      continue;
    }
    
    // On vérifie que l'on a bien "0x" au début
    if (content.substring(tokenStart, tokenStart + 2) != "0x") {
      searchIndex = tokenStart + 1;
      continue;
    }
    
    // Déterminer la fin du token hexadécimal
    int tokenEnd = tokenStart + 2; // on saute "0x"
    while (tokenEnd < content.length() && isHexDigit(content.charAt(tokenEnd))) {
      tokenEnd++;
    }
    
    // Insérer des guillemets autour de la valeur hexadécimale
    // Par exemple, transforme : 0x24 en : "0x24"
    String hexToken = content.substring(tokenStart, tokenEnd);
    String quotedHex = "\"" + hexToken + "\"";
    
    // Reconstruire la chaîne en insérant la valeur corrigée
    content = content.substring(0, tokenStart) + quotedHex + content.substring(tokenEnd);
    
    // Avancer searchIndex pour continuer la recherche après le token inséré
    searchIndex = tokenStart + quotedHex.length();
  }
  return content;
}