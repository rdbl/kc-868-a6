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

// --- Vérification d'existence d'une clé ---
bool DataStore::exists(const String& key) {
    return doc[key].is<JsonObject>() && !doc[key]["value"].isNull();
}

// --- Récupération de l'objet complet associé à une clé ---
JsonObject DataStore::getObject(const String& key) {
    if (doc[key].is<JsonObject>()) {
        return doc[key].as<JsonObject>();
    }
    return JsonObject(); // Retourne un objet JSON vide si la clé n'existe pas
}

// --- Méthodes de gestion des types ---
DataType DataStore::getValueType(const String& key) {
    if (!exists(key)) {
        return DataType::UNKNOWN;
    }
    
    JsonObject entry = doc[key].as<JsonObject>();
    if (!entry["type"].is<String>()) {
        return DataType::UNKNOWN;
    }
    
    return stringToDataType(entry["type"].as<String>());
}

bool DataStore::isOfType(const String& key, DataType expectedType) {
    return getValueType(key) == expectedType;
}

// --- Méthodes pour les timestamps ---
unsigned long DataStore::getLastUpdateTick(const String& key) {
    if (!exists(key)) {
        return 0;
    }
    
    JsonObject entry = doc[key].as<JsonObject>();
    if (!entry["tricks"].is<unsigned long>()) {
        return 0;
    }
    
    return entry["tricks"].as<unsigned long>();
}

bool DataStore::isStale(const String& key, unsigned long maxAgeTicks) {
    if (!exists(key)) {
        return true; // Si la donnée n'existe pas, elle est considérée comme obsolète
    }
    
    unsigned long lastUpdate = getLastUpdateTick(key);
    unsigned long currentTicks = millis();
    
    // Gestion du débordement de millis()
    if (currentTicks < lastUpdate) {
        // millis() a débordé et est revenu à 0
        return (ULONG_MAX - lastUpdate + currentTicks) > maxAgeTicks;
    }
    
    return (currentTicks - lastUpdate) > maxAgeTicks;
}

// --- Affichage du contenu ---
void DataStore::printStore() {
    Serial.println("========== [ DataStore Dump ] ==========");
    String output;
    // Filtre les objets pour n'afficher que les valeurs
    for (auto entry : doc.as<JsonObject>()) {
        if (entry.value().is<JsonObject>()) {
            JsonObject obj = entry.value().as<JsonObject>();
            if (obj["value"].is<JsonVariant>()) {
                String type = obj["type"].as<String>();
                unsigned long lastUpdate = obj["tricks"].as<unsigned long>();
                unsigned long age = millis() - lastUpdate;
                
                output += String(entry.key().c_str()) + ": " + obj["value"].as<String>() + 
                          " (" + type + ") - Dernière mise à jour: " + String(age) + " ms\n";
            }
        }
    }
    Serial.println(output);
    Serial.println("=========================================");
}

// --- Conversion en JSON ---
JsonDocument& DataStore::toJson() {
    return doc;
}

// --- Système d'abonnement ---
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

// (Pour la fonction template getWithTypeCheck<>)
template int DataStore::getWithTypeCheck<int>(const String&, int);
template float DataStore::getWithTypeCheck<float>(const String&, float);
template bool DataStore::getWithTypeCheck<bool>(const String&, bool);
template String DataStore::getWithTypeCheck<String>(const String&, String);
template const char* DataStore::getWithTypeCheck<const char*>(const String&, const char*);
template double DataStore::getWithTypeCheck<double>(const String&, double);

// (Pour la fonction template isOfType<>)
template bool DataStore::isOfType<int>(const String&);
template bool DataStore::isOfType<unsigned int>(const String&);
template bool DataStore::isOfType<long>(const String&);
template bool DataStore::isOfType<unsigned long>(const String&);
template bool DataStore::isOfType<float>(const String&);
template bool DataStore::isOfType<double>(const String&);
template bool DataStore::isOfType<bool>(const String&);
template bool DataStore::isOfType<String>(const String&);
template bool DataStore::isOfType<const char*>(const String&);