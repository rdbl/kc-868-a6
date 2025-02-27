#include "DataStore.h"
#include <ArduinoYaml.h>
#include <SPIFFS.h>

DataStore::DataStore() {
    doc.clear();
}

// Singleton
DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

// =========================
// 🔹 GET & SET Génériques avec callback
// =========================
template <typename T>
T DataStore::get(const String& key, T defaultValue) {
    JsonVariant value = doc[key];
    if (!value.isNull()) {
        return value.as<T>();
    }
    return defaultValue;
}

template <typename T>
void DataStore::set(const String& key, T value) {
    if (doc[key] == value) return;

    doc[key] = value;

    if (callbacks.find(key) != callbacks.end()) {
        Serial.printf("[EVENT] Changement détecté sur '%s', exécution du callback\n", key.c_str());
        callbacks[key]();
    }
}

// =========================
// 🔹 Chargement de `configuration.yaml` dans le DataStore
// =========================
void DataStore::loadFromConfig() {
    Serial.println("[INFO] Chargement direct de `configuration.yaml` dans DataStore...");

    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] Échec de l'initialisation SPIFFS !");
        return;
    }

    File file = SPIFFS.open("/configuration.yaml", "r");
    if (!file) {
        Serial.println("[ERROR] Impossible d'ouvrir `configuration.yaml` !");
        return;
    }

    // Lecture du fichier YAML
    String yamlContent = file.readString();
    file.close();

    // Conversion YAML -> JSON
    YAMLNode yamlNode = YAMLNode::loadString(yamlContent.c_str());
    String jsonContent;
    serializeYml(yamlNode.getDocument(), jsonContent, OUTPUT_JSON);

    // Désérialisation JSON -> DataStore
    DeserializationError error = deserializeJson(doc["InitStore"], jsonContent);
    if (error) {
        Serial.print("[ERROR] Erreur lors de la conversion YAML -> JSON: ");
        Serial.println(error.c_str());
        return;
    }

    Serial.println("[INFO] `configuration.yaml` chargé dans DataStore avec succès !");
}

void DataStore::printStore() {
    Serial.println("========== [ DataStore Dump ] ==========");
    
    // Sérialisation du JSON pour affichage
    String output;
    serializeJsonPretty(doc, output);
    Serial.println(output);

    Serial.println("=========================================");
}


// =========================
// 🔹 Gestion des événements (Callbacks)
// =========================
void DataStore::subscribe(const String& key, std::function<void()> callback) {
    callbacks[key] = callback;
}

// =========================
// 🔹 Conversion JSON
// =========================
JsonDocument& DataStore::toJson() {
    return doc;
}

// Spécialisations explicites pour éviter des erreurs de linkage (C++)
// Spécialisations explicites pour éviter des erreurs de linkage (C++)
template int DataStore::get<int>(const String&, int);
template float DataStore::get<float>(const String&, float);
template bool DataStore::get<bool>(const String&, bool);
template String DataStore::get<String>(const String&, String);
template const char* DataStore::get<const char*>(const String&, const char*);
template double DataStore::get<double>(const String&, double);

template void DataStore::set<int>(const String&, int);
template void DataStore::set<float>(const String&, float);
template void DataStore::set<bool>(const String&, bool);
template void DataStore::set<String>(const String&, String);
template void DataStore::set<const char*>(const String&, const char*);
template void DataStore::set<double>(const String&, double);

