#ifndef LOADER_CONFIG_H
#define LOADER_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "EventManager.h"
#include "DataStore.h"

/**
 * @brief Classe pour charger la configuration et la distribuer via EventManager
 * 
 * Cette classe va:
 * - Charger la configuration depuis un fichier YAML
 * - Stocker les données dans le DataStore
 * - Déclencher des événements pour notifier les modules de la disponibilité des données
 */
class ConfigLoader {
public:
    /**
     * @brief Obtenir l'instance unique du ConfigLoader (Singleton)
     * @return ConfigLoader& - Référence vers l'instance unique
     */
    static ConfigLoader& getInstance();

    /**
     * @brief Initialiser et charger la configuration
     * @param configPath Chemin du fichier de configuration (par défaut: "/configuration.yaml")
     * @return bool - true si le chargement a réussi, false sinon
     */
    bool init(const String& configPath = "/configuration.yaml");

    /**
     * @brief Recharger la configuration depuis le fichier
     * @return bool - true si le rechargement a réussi, false sinon
     */
    bool reload();

    /**
     * @brief Vérifier si la configuration est valide
     * @return bool - true si la configuration est valide, false sinon
     */
    bool isValid() const;

    /**
     * @brief Obtenir les informations sur les erreurs de validation
     * @return String - Message décrivant les erreurs de validation
     */
    String getValidationErrors() const;

private:
    // Constructeur privé (Singleton)
    ConfigLoader();
    // Empêcher la copie et l'assignation
    ConfigLoader(const ConfigLoader&) = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;

    // Méthodes privées pour le chargement et la validation
    bool loadFromYaml(const String& yamlContent);
    bool loadFromJson(const String& jsonContent);
    bool validateConfig(const JsonDocument& doc);
    
    // Méthodes pour publier les données dans le DataStore - utilisant JsonVariantConst pour ArduinoJson 7
    void publishConfigToDataStore(const JsonDocument& doc);
    void publishWifiConfig(const JsonVariantConst& wifiObj);
    void publishMqttConfig(const JsonVariantConst& mqttObj);
    void publishOtaConfig(const JsonVariantConst& otaObj);
    void publishDeviceConfig(const JsonVariantConst& deviceObj);
    void publishSensorsConfig(const JsonVariantConst& sensorsArray);
    void publishActuatorsConfig(const JsonVariantConst& actuatorsArray);
    void publishDisplaysConfig(const JsonVariantConst& displaysArray);

    // Chemin du fichier de configuration
    String configPath_;
    // Erreurs de validation
    String validationErrors_;
    // État de la validation
    bool isValid_;
    // Référence à l'EventManager
    EventManager& eventManager_;
    // Référence au DataStore
    DataStore& dataStore_;
};

#endif // LOADER_CONFIG_H