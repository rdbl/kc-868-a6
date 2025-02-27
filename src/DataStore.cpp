#include "DataStore.h"
#include <ArduinoYaml.h>
#include <SPIFFS.h>

// --- Implémentation du Singleton ---
DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

DataStore::DataStore() {
    doc.clear(); // S'assurer que le document est vide au démarrage
}

// --- Implémentation du getter générique ---
template <typename T>
T DataStore::get(const String& key, T defaultValue) {
    // Utiliser directement doc[key].is<JsonObject>() au lieu de containsKey()
    if (doc[key].is<JsonObject>()) {
        JsonObject entry = doc[key].as<JsonObject>();
        // Vérifier si "value" existe (remplace containsKey() par une vérification de nullité)
        if (!entry["value"].isNull()) {
            return entry["value"].as<T>();
        }
    }
    return defaultValue;
}

// --- Implémentation du setter générique ---
// L'entrée est stockée sous forme d'objet JSON contenant "value", "type" et "tricks"
template <typename T>
void DataStore::set(const String& key, T value) {
    JsonObject entry;
    // Créer ou récupérer l'objet associé à la clé
    if (doc[key].is<JsonObject>()) {
        entry = doc[key].as<JsonObject>();
    } else {
        // Remplacer createNestedObject() déprécié par to<JsonObject>()
        entry = doc[key].to<JsonObject>();
    }

    // Vérifier si "value" est déjà présente et égale à la nouvelle valeur
    if (!entry["value"].isNull() && entry["value"] == value) {
        return;
    }

    // Mise à jour de l'entrée
    entry["value"] = value;
    entry["type"] = getTypeName(value);
    entry["tricks"] = millis();

    // Exécution des callbacks abonnés (s'ils existent)
    if (callbacks.find(key) != callbacks.end()) {
        Serial.printf("[DATASTORE] Changement détecté sur '%s', exécution des callbacks\n", key.c_str());
        for (auto& cb : callbacks[key]) {
            cb(entry["value"]);  // Transmission de la nouvelle valeur
        }
    }
}

// --- Récupération de l'objet complet associé à une clé ---
JsonObject DataStore::getObject(const String& key) {
    if (doc[key].is<JsonObject>()) {
        return doc[key].as<JsonObject>();
    }
    return JsonObject(); // Retourne un objet JSON vide si la clé n'existe pas
}

void DataStore::loadFromConfig() {
    // À implémenter dans une étape ultérieure (chargement de configuration YAML)
    Serial.println("[INFO] Chargement de la configuration (étape à venir)");
}

void DataStore::printStore() {
    Serial.println("========== [ DataStore Dump ] ==========");
    String output;
    // filtre les objets pour n'afficher que les valeurs . creer un nouvelle objet json avec uniquement les values
    // serializeJsonPretty(doc, output);
    for (auto entry : doc.as<JsonObject>()) {
        if (entry.value().is<JsonObject>()) {
            JsonObject obj = entry.value().as<JsonObject>();
            if (obj["value"].is<JsonVariant>()) {
                output += String(entry.key().c_str()) + ": " + obj["value"].as<String>() + " (" + obj["type"].as<String>() + ")\n";
            }
        }
    }
    Serial.println(output);
    Serial.println("=========================================");
}

JsonDocument& DataStore::toJson() {
    return doc;
}

void DataStore::subscribe(const String& key, std::function<void(JsonVariant)> callback) {
    callbacks[key].push_back(callback);
}

// --- Spécialisations explicites pour éviter des erreurs de linkage ---
// (Pour la fonction template get<>)
template int DataStore::get<int>(const String&, int);
template float DataStore::get<float>(const String&, float);
template bool DataStore::get<bool>(const String&, bool);
template String DataStore::get<String>(const String&, String);
template const char* DataStore::get<const char*>(const String&, const char*);
template double DataStore::get<double>(const String&, double);

// (Pour la fonction template set<>)
template void DataStore::set<int>(const String&, int);
template void DataStore::set<float>(const String&, float);
template void DataStore::set<bool>(const String&, bool);
template void DataStore::set<String>(const String&, String);
template void DataStore::set<const char*>(const String&, const char*);
template void DataStore::set<double>(const String&, double);
