#include "DataStore.h"

// Implémentation du Singleton
DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

// Constructeur privé
DataStore::DataStore() {
    // Initialisation des membres si nécessaire
}

// Méthode exists
bool DataStore::exists(const String& key) {
    return doc[key].is<JsonObject>() && !doc[key]["value"].isNull();
}

// Récupération de l'objet associé à la clé
JsonObject DataStore::getObject(const String& key) {
    if (doc[key].is<JsonObject>()) {
        return doc[key].as<JsonObject>();
    }
    return JsonObject(); // Retourne un objet JSON vide si la clé n'existe pas
}

// Méthodes de gestion des types
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

// Méthode pour obtenir le timestamp
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

// Vérification d'obsolescence
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

// Affichage du contenu du store
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

// Conversion en JSON
JsonDocument& DataStore::toJson() {
    return doc;
}

// Abonnement à une clé
void DataStore::subscribe(const String& key, std::function<void(JsonVariant)> callback) {
    callbacks[key].push_back(callback);
}

// ===============================================================
// Instantiations explicites des templates
// ===============================================================

// Pour la méthode get avec la nouvelle signature (bool get(const String&, T&))
template bool DataStore::get<int>(const String&, int&);
template bool DataStore::get<float>(const String&, float&);
template bool DataStore::get<bool>(const String&, bool&);
template bool DataStore::get<String>(const String&, String&);
template bool DataStore::get<const char*>(const String&, const char*&);
template bool DataStore::get<double>(const String&, double&);
template bool DataStore::get<unsigned int>(const String&, unsigned int&);
template bool DataStore::get<long>(const String&, long&);
template bool DataStore::get<unsigned long>(const String&, unsigned long&);

// Pour la méthode set (inchangée)
template void DataStore::set<int>(const String&, int);
template void DataStore::set<float>(const String&, float);
template void DataStore::set<bool>(const String&, bool);
template void DataStore::set<String>(const String&, String);
template void DataStore::set<const char*>(const String&, const char*);
template void DataStore::set<double>(const String&, double);
template void DataStore::set<unsigned int>(const String&, unsigned int);
template void DataStore::set<long>(const String&, long);
template void DataStore::set<unsigned long>(const String&, unsigned long);

// Pour la méthode isOfType (si nécessaire)
template bool DataStore::isOfType<int>(const String&);
template bool DataStore::isOfType<unsigned int>(const String&);
template bool DataStore::isOfType<long>(const String&);
template bool DataStore::isOfType<unsigned long>(const String&);
template bool DataStore::isOfType<float>(const String&);
template bool DataStore::isOfType<double>(const String&);
template bool DataStore::isOfType<bool>(const String&);
template bool DataStore::isOfType<String>(const String&);
template bool DataStore::isOfType<const char*>(const String&);