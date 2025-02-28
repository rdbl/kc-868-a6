#include "LoaderConfig.h"
#include <ArduinoYaml.h>

// Implémentation du Singleton
ConfigLoader& ConfigLoader::getInstance() {
    static ConfigLoader instance;
    return instance;
}

// Constructeur
ConfigLoader::ConfigLoader()
    : configPath_("/configuration.yaml"), 
      isValid_(false),
      eventManager_(EventManager::getInstance()),
      dataStore_(DataStore::getInstance()) {
    // Initialisation des champs vides
    validationErrors_ = "";
}

// Initialisation avec chargement du fichier
bool ConfigLoader::init(const String& configPath) {
    configPath_ = configPath;
    
    // Monter le système de fichiers SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("[LOADER] Erreur lors du montage de SPIFFS");
        validationErrors_ = "Impossible de monter SPIFFS";
        return false;
    }
    
    return reload();
}

// Rechargement de la configuration
bool ConfigLoader::reload() {
    if (!SPIFFS.exists(configPath_)) {
        Serial.println("[LOADER] Fichier de configuration non trouvé: " + configPath_);
        validationErrors_ = "Fichier de configuration non trouvé: " + configPath_;
        return false;
    }
    
    File configFile = SPIFFS.open(configPath_, "r");
    if (!configFile) {
        Serial.println("[LOADER] Impossible d'ouvrir le fichier de configuration");
        validationErrors_ = "Impossible d'ouvrir le fichier de configuration";
        return false;
    }
    
    String fileContent = configFile.readString();
    configFile.close();
    
    // Détecter si c'est du YAML ou du JSON en fonction de l'extension
    bool result = false;
    if (configPath_.endsWith(".yaml") || configPath_.endsWith(".yml")) {
        result = loadFromYaml(fileContent);
    } else if (configPath_.endsWith(".json")) {
        result = loadFromJson(fileContent);
    } else {
        Serial.println("[LOADER] Format de fichier non reconnu");
        validationErrors_ = "Format de fichier non reconnu";
        return false;
    }
    
    return result;
}

// Charger depuis YAML
bool ConfigLoader::loadFromYaml(const String& yamlContent) {
    Serial.println("[LOADER] Chargement de la configuration YAML");
    
    // Utiliser la bibliothèque ArduinoYaml pour convertir en JSON
    YAMLNode yamlnode = YAMLNode::loadString(yamlContent.c_str());
    // Vérifier si yamlnode est valide (correction de l'opérateur !)
    if (yamlnode.getDocument() == nullptr) {
        Serial.println("[LOADER] Erreur lors du parsing YAML");
        validationErrors_ = "Erreur lors du parsing YAML";
        return false;
    }
    
    String jsonContent;
    serializeYml(yamlnode.getDocument(), jsonContent, OUTPUT_JSON);
    
    // Maintenant charger à partir du JSON
    return loadFromJson(jsonContent);
}

// Charger depuis JSON
bool ConfigLoader::loadFromJson(const String& jsonContent) {
    Serial.println("[LOADER] Parsing du JSON");
    
    // Déserialisation du JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        Serial.print("[LOADER] Erreur de désérialisation JSON: ");
        Serial.println(error.c_str());
        validationErrors_ = "Erreur de désérialisation JSON: ";
        validationErrors_ += error.c_str();
        return false;
    }
    
    // Valider la configuration
    if (!validateConfig(doc)) {
        Serial.println("[LOADER] La configuration n'est pas valide");
        return false;
    }
    
    // Publier les données dans le DataStore
    publishConfigToDataStore(doc);
    
    Serial.println("[LOADER] Configuration chargée avec succès");
    
    // Notifier que la configuration est chargée
    eventManager_.publish("ConfigLoaded");
    
    return true;
}

// Validation de la configuration
bool ConfigLoader::validateConfig(const JsonDocument& doc) {
    isValid_ = true;
    validationErrors_ = "";
    
    // Vérifier la présence des sections obligatoires
    if (doc["device"].isNull()) {
        isValid_ = false;
        validationErrors_ += "Erreur: Section 'device' manquante dans la configuration\n";
    }
    
    // Vérifier le device
    if (!doc["device"]["id"].isNull() && doc["device"]["id"].as<String>().isEmpty()) {
        isValid_ = false;
        validationErrors_ += "Erreur: Le device n'a pas d'ID\n";
    }
    
    // Vérifier les capteurs (avec vérification plus sécurisée)
    if (!doc["device"]["sensors"].isNull()) {
        // Pour ArduinoJson 7, nous devons itérer directement
        int index = 0;
        for (JsonVariantConst sensor : doc["device"]["sensors"].as<JsonArrayConst>()) {
            if (sensor["id"].isNull() || sensor["id"].as<String>().isEmpty()) {
                isValid_ = false;
                validationErrors_ += "Erreur: Le capteur " + String(index) + " n'a pas d'ID\n";
            }
            index++;
        }
    }
    
    // Vérifier les actionneurs
    if (!doc["device"]["actuators"].isNull()) {
        int index = 0;
        for (JsonVariantConst actuator : doc["device"]["actuators"].as<JsonArrayConst>()) {
            if (actuator["id"].isNull() || actuator["id"].as<String>().isEmpty()) {
                isValid_ = false;
                validationErrors_ += "Erreur: L'actionneur " + String(index) + " n'a pas d'ID\n";
            }
            index++;
        }
    }
    
    // Vérifier les afficheurs
    if (!doc["device"]["displays"].isNull()) {
        int index = 0;
        for (JsonVariantConst display : doc["device"]["displays"].as<JsonArrayConst>()) {
            if (display["id"].isNull() || display["id"].as<String>().isEmpty()) {
                isValid_ = false;
                validationErrors_ += "Erreur: L'afficheur " + String(index) + " n'a pas d'ID\n";
            }
            index++;
        }
    }
    
    return isValid_;
}

// Publication de la configuration dans le DataStore
void ConfigLoader::publishConfigToDataStore(const JsonDocument& doc) {
    // Publier le mode simulation
    dataStore_.set("SimulationMode", doc["SIMULATION_MODE"] | false);
    
    // Publier les configurations spécifiques
    if (!doc["wifi"].isNull()) {
        publishWifiConfig(doc["wifi"]);
    }
    
    if (!doc["mqtt"].isNull()) {
        publishMqttConfig(doc["mqtt"]);
    }
    
    if (!doc["ota"].isNull()) {
        publishOtaConfig(doc["ota"]);
    }
    
    if (!doc["device"].isNull()) {
        publishDeviceConfig(doc["device"]);
        
        // Publier les capteurs, actionneurs et afficheurs
        if (!doc["device"]["sensors"].isNull()) {
            publishSensorsConfig(doc["device"]["sensors"]);
        }
        
        if (!doc["device"]["actuators"].isNull()) {
            publishActuatorsConfig(doc["device"]["actuators"]);
        }
        
        if (!doc["device"]["displays"].isNull()) {
            publishDisplaysConfig(doc["device"]["displays"]);
        }
    }
}

// Publication de la configuration WiFi
void ConfigLoader::publishWifiConfig(const JsonVariantConst& wifiObj) {
    // Stocker les éléments individuels
    dataStore_.set("WiFi.SSID", wifiObj["ssid"] | "");
    dataStore_.set("WiFi.Password", wifiObj["password"] | "");
    
    // Stocker l'objet complet au format JSON
    String wifiJson;
    serializeJson(wifiObj, wifiJson);
    dataStore_.set("WiFi.Config", wifiJson);
    
    // Signaler que la configuration WiFi est disponible
    eventManager_.publish("WiFiConfigAvailable");
}

// Publication de la configuration MQTT
void ConfigLoader::publishMqttConfig(const JsonVariantConst& mqttObj) {
    // Stocker les éléments individuels
    dataStore_.set("MQTT.Host", mqttObj["host"] | "");
    dataStore_.set("MQTT.Port", mqttObj["port"] | 1883);
    dataStore_.set("MQTT.User", mqttObj["user"] | "");
    dataStore_.set("MQTT.Password", mqttObj["password"] | "");
    dataStore_.set("MQTT.Topic", mqttObj["topic"] | "");
    
    // Stocker l'objet complet au format JSON
    String mqttJson;
    serializeJson(mqttObj, mqttJson);
    dataStore_.set("MQTT.Config", mqttJson);
    
    // Signaler que la configuration MQTT est disponible
    eventManager_.publish("MQTTConfigAvailable");
}

// Publication de la configuration OTA
void ConfigLoader::publishOtaConfig(const JsonVariantConst& otaObj) {
    // Stocker les éléments individuels
    dataStore_.set("OTA.Enabled", otaObj["enabled"] | false);
    dataStore_.set("OTA.Port", otaObj["port"] | 3232);
    
    // Stocker l'objet complet au format JSON
    String otaJson;
    serializeJson(otaObj, otaJson);
    dataStore_.set("OTA.Config", otaJson);
    
    // Signaler que la configuration OTA est disponible
    eventManager_.publish("OTAConfigAvailable");
}

// Publication de la configuration du device
void ConfigLoader::publishDeviceConfig(const JsonVariantConst& deviceObj) {
    // Stocker les informations de base du device
    dataStore_.set("Device.ID", deviceObj["id"] | "");
    
    // Stocker les informations du header
    if (!deviceObj["header"].isNull()) {
        JsonVariantConst header = deviceObj["header"];
        dataStore_.set("Device.Name", header["name"] | "");
        dataStore_.set("Device.Model", header["model"] | "");
        dataStore_.set("Device.Description", header["description"] | "");
        dataStore_.set("Device.Room", header["room"] | "");
    }
    
    // Stocker les informations hardware
    if (!deviceObj["hardware"].isNull()) {
        JsonVariantConst hardware = deviceObj["hardware"];
        String hardwareJson;
        serializeJson(hardware, hardwareJson);
        dataStore_.set("Device.Hardware", hardwareJson);
        
        // Stocker les informations I2C si présentes
        if (!hardware["i2c"].isNull()) {
            JsonVariantConst i2c = hardware["i2c"];
            dataStore_.set("Device.I2C.SDA", i2c["sda"] | 0);
            dataStore_.set("Device.I2C.SCL", i2c["scl"] | 0);
            
            // Stocker les informations du PCF8574 si présentes
            if (!i2c["pcf8574"].isNull()) {
                JsonVariantConst pcf = i2c["pcf8574"];
                dataStore_.set("Device.PCF8574.Address", pcf["address_hex"] | "0x20");
                dataStore_.set("Device.PCF8574.NumRelays", pcf["num_relays"] | 0);
            }
        }
    }
    
    // Stocker l'objet complet au format JSON
    String deviceJson;
    serializeJson(deviceObj, deviceJson);
    dataStore_.set("Device.Config", deviceJson);
    
    // Signaler que la configuration du device est disponible
    eventManager_.publish("DeviceConfigAvailable");
}

// Publication de la configuration des capteurs
void ConfigLoader::publishSensorsConfig(const JsonVariantConst& sensorsArray) {
    int sensorCount = 0;
    
    for (JsonVariantConst sensor : sensorsArray.as<JsonArrayConst>()) {
        // Préfixe pour les clés du capteur
        String prefix = "Sensor." + String(sensorCount) + ".";
        String id = sensor["id"] | "";
        
        // Stocker l'ID et le préfixe dans une table de correspondance
        dataStore_.set("SensorID." + id, sensorCount);
        
        // Stocker les informations de base du capteur
        dataStore_.set(prefix + "ID", id);
        
        // Stocker les informations du header
        if (!sensor["header"].isNull()) {
            JsonVariantConst header = sensor["header"];
            dataStore_.set(prefix + "Name", header["name"] | "");
            dataStore_.set(prefix + "Model", header["model"] | "");
            dataStore_.set(prefix + "Description", header["description"] | "");
        }
        
        // Stocker les informations hardware
        if (!sensor["hardware"].isNull()) {
            JsonVariantConst hardware = sensor["hardware"];
            String hardwareJson;
            serializeJson(hardware, hardwareJson);
            dataStore_.set(prefix + "Hardware", hardwareJson);
            
            // Stocker les informations des pins si présentes
            if (!hardware["pins"].isNull()) {
                JsonVariantConst pins = hardware["pins"];
                dataStore_.set(prefix + "Pin", pins["COM"] | 0);
            }
        }
        
        // Stocker les données opérationnelles
        if (!sensor["data"].isNull()) {
            JsonVariantConst data = sensor["data"];
            String dataJson;
            serializeJson(data, dataJson);
            dataStore_.set(prefix + "Data", dataJson);
            
            // Stocker les paramètres spécifiques
            dataStore_.set(prefix + "Temperature", data["temperature"] | 0.0f);
            dataStore_.set(prefix + "Unit", data["unit"] | "°C");
            dataStore_.set(prefix + "MaxTemp", data["max_temp"] | 0.0f);
            dataStore_.set(prefix + "DelaySeconds", data["delay_seconds"] | 0);
        }
        
        // Stocker l'objet complet au format JSON
        String sensorJson;
        serializeJson(sensor, sensorJson);
        dataStore_.set(prefix + "Config", sensorJson);
        
        // Publier un événement pour ce capteur spécifique
        eventManager_.publish("SensorConfigAvailable." + id);
        
        sensorCount++;
    }
    
    // Stocker le nombre total de capteurs
    dataStore_.set("Sensor.Count", sensorCount);
    
    // Signaler que tous les capteurs sont configurés
    eventManager_.publish("AllSensorsConfigAvailable");
}

// Publication de la configuration des actionneurs
void ConfigLoader::publishActuatorsConfig(const JsonVariantConst& actuatorsArray) {
    int actuatorCount = 0;
    
    for (JsonVariantConst actuator : actuatorsArray.as<JsonArrayConst>()) {
        // Préfixe pour les clés de l'actionneur
        String prefix = "Actuator." + String(actuatorCount) + ".";
        String id = actuator["id"] | "";
        
        // Stocker l'ID et le préfixe dans une table de correspondance
        dataStore_.set("ActuatorID." + id, actuatorCount);
        
        // Stocker les informations de base de l'actionneur
        dataStore_.set(prefix + "ID", id);
        
        // Stocker les informations du header
        if (!actuator["header"].isNull()) {
            JsonVariantConst header = actuator["header"];
            dataStore_.set(prefix + "Name", header["name"] | "");
            dataStore_.set(prefix + "Model", header["model"] | "");
            dataStore_.set(prefix + "Description", header["description"] | "");
        }
        
        // Stocker les informations hardware
        if (!actuator["hardware"].isNull()) {
            JsonVariantConst hardware = actuator["hardware"];
            String hardwareJson;
            serializeJson(hardware, hardwareJson);
            dataStore_.set(prefix + "Hardware", hardwareJson);
            
            // Stocker les informations I2C si présentes
            if (!hardware["i2c"].isNull()) {
                dataStore_.set(prefix + "I2CPin", hardware["i2c"] | "");
            }
        }
        
        // Stocker les données opérationnelles
        if (!actuator["data"].isNull()) {
            JsonVariantConst data = actuator["data"];
            String dataJson;
            serializeJson(data, dataJson);
            dataStore_.set(prefix + "Data", dataJson);
            
            // Stocker les paramètres spécifiques
            dataStore_.set(prefix + "State", data["state"] | "off");
            dataStore_.set(prefix + "Mode", data["mode"] | "AUTO");
            dataStore_.set(prefix + "DiffStart", data["diff_start"] | 0.0f);
            dataStore_.set(prefix + "DiffStop", data["diff_stop"] | 0.0f);
            dataStore_.set(prefix + "RelayDelaySeconds", data["relay_delay_seconds"] | 0);
        }
        
        // Stocker l'objet complet au format JSON
        String actuatorJson;
        serializeJson(actuator, actuatorJson);
        dataStore_.set(prefix + "Config", actuatorJson);
        
        // Publier un événement pour cet actionneur spécifique
        eventManager_.publish("ActuatorConfigAvailable." + id);
        
        actuatorCount++;
    }
    
    // Stocker le nombre total d'actionneurs
    dataStore_.set("Actuator.Count", actuatorCount);
    
    // Signaler que tous les actionneurs sont configurés
    eventManager_.publish("AllActuatorsConfigAvailable");
}

// Publication de la configuration des afficheurs
void ConfigLoader::publishDisplaysConfig(const JsonVariantConst& displaysArray) {
    int displayCount = 0;
    
    for (JsonVariantConst display : displaysArray.as<JsonArrayConst>()) {
        // Préfixe pour les clés de l'afficheur
        String prefix = "Display." + String(displayCount) + ".";
        String id = display["id"] | "";
        
        // Stocker l'ID et le préfixe dans une table de correspondance
        dataStore_.set("DisplayID." + id, displayCount);
        
        // Stocker les informations de base de l'afficheur
        dataStore_.set(prefix + "ID", id);
        
        // Stocker les informations du header
        if (!display["header"].isNull()) {
            JsonVariantConst header = display["header"];
            dataStore_.set(prefix + "Name", header["name"] | "");
            dataStore_.set(prefix + "Model", header["model"] | "");
            dataStore_.set(prefix + "Description", header["description"] | "");
        }
        
        // Stocker les informations hardware
        if (!display["hardware"].isNull()) {
            JsonVariantConst hardware = display["hardware"];
            String hardwareJson;
            serializeJson(hardware, hardwareJson);
            dataStore_.set(prefix + "Hardware", hardwareJson);
            
            // Stocker les informations des pins si présentes
            if (!hardware["pins"].isNull()) {
                JsonVariantConst pins = hardware["pins"];
                dataStore_.set(prefix + "Pin", pins["COM"] | 0);
            }
        }
        
        // Stocker les données opérationnelles
        if (!display["data"].isNull()) {
            JsonVariantConst data = display["data"];
            String dataJson;
            serializeJson(data, dataJson);
            dataStore_.set(prefix + "Data", dataJson);
            
            // Stocker les paramètres spécifiques
            if (!data["size"].isNull()) {
                JsonVariantConst size = data["size"];
                dataStore_.set(prefix + "SizeX", size["x"] | 0);
                dataStore_.set(prefix + "SizeY", size["y"] | 0);
            }
            
            // Pour les afficheurs de type série
            dataStore_.set(prefix + "Bauds", data["bauds"] | 0);
            dataStore_.set(prefix + "Verbose", data["verbose"] | "");
        }
        
        // Stocker l'objet complet au format JSON
        String displayJson;
        serializeJson(display, displayJson);
        dataStore_.set(prefix + "Config", displayJson);
        
        // Publier un événement pour cet afficheur spécifique
        eventManager_.publish("DisplayConfigAvailable." + id);
        
        displayCount++;
    }
    
    // Stocker le nombre total d'afficheurs
    dataStore_.set("Display.Count", displayCount);
    
    // Signaler que tous les afficheurs sont configurés
    eventManager_.publish("AllDisplaysConfigAvailable");
}

// Accesseurs
bool ConfigLoader::isValid() const {
    return isValid_;
}

String ConfigLoader::getValidationErrors() const {
    return validationErrors_;
}