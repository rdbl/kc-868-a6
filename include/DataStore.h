#ifndef DATASTORE_H
#define DATASTORE_H

#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <functional>

// --- Enumération des types de données supportés ---
enum class DataType {
    UNKNOWN,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE,
    BOOL,
    STRING,
    CHAR_PTR
};

// --- Fonctions de conversion entre DataType et String ---
inline String dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::UINT: return "unsigned int";
        case DataType::LONG: return "long";
        case DataType::ULONG: return "unsigned long";
        case DataType::FLOAT: return "float";
        case DataType::DOUBLE: return "double";
        case DataType::BOOL: return "bool";
        case DataType::STRING: return "String";
        case DataType::CHAR_PTR: return "const char*";
        default: return "unknown";
    }
}

inline DataType stringToDataType(const String& typeStr) {
    if (typeStr == "int") return DataType::INT;
    if (typeStr == "unsigned int") return DataType::UINT;
    if (typeStr == "long") return DataType::LONG;
    if (typeStr == "unsigned long") return DataType::ULONG;
    if (typeStr == "float") return DataType::FLOAT;
    if (typeStr == "double") return DataType::DOUBLE;
    if (typeStr == "bool") return DataType::BOOL;
    if (typeStr == "String") return DataType::STRING;
    if (typeStr == "const char*") return DataType::CHAR_PTR;
    return DataType::UNKNOWN;
}

// --- Obtenir le DataType d'une variable via spécialisation de template ---
template <typename T> inline DataType getDataType() { return DataType::UNKNOWN; }
template <> inline DataType getDataType<int>() { return DataType::INT; }
template <> inline DataType getDataType<unsigned int>() { return DataType::UINT; }
template <> inline DataType getDataType<long>() { return DataType::LONG; }
template <> inline DataType getDataType<unsigned long>() { return DataType::ULONG; }
template <> inline DataType getDataType<float>() { return DataType::FLOAT; }
template <> inline DataType getDataType<double>() { return DataType::DOUBLE; }
template <> inline DataType getDataType<bool>() { return DataType::BOOL; }
template <> inline DataType getDataType<String>() { return DataType::STRING; }
template <> inline DataType getDataType<const char*>() { return DataType::CHAR_PTR; }

// --- Déclaration de la classe DataStore ---
class DataStore {
public:
    static DataStore& getInstance();  // Singleton

    // Vérifier si une clé existe dans le store
    bool exists(const String& key);

    // Getter générique : retourne la valeur stockée (extraite du champ "value")
    template <typename T>
    T get(const String& key, T defaultValue = T());

    // Getter sécurisé avec vérification de type
    template <typename T>
    T getWithTypeCheck(const String& key, T defaultValue = T());

    // Setter générique : encapsule la donnée dans un objet JSON contenant "value", "type" et "tricks"
    template <typename T>
    void set(const String& key, T value);

    // Récupère l'objet complet associé à la clé (contenant "value", "type" et "tricks")
    JsonObject getObject(const String& key);

    // Méthodes de gestion des types
    DataType getValueType(const String& key);
    bool isOfType(const String& key, DataType expectedType);
    
    template <typename T>
    bool isOfType(const String& key) {
        return isOfType(key, getDataType<T>());
    }

    // Méthode pour obtenir le timestamp (ticks) de la dernière mise à jour
    unsigned long getLastUpdateTick(const String& key);
    
    // Méthode pour vérifier si une donnée est obsolète (plus vieille qu'un certain nombre de ticks)
    bool isStale(const String& key, unsigned long maxAgeTicks);

    // Affichage du contenu du DataStore
    void printStore();

    // Conversion en JSON pour MQTT et affichage
    JsonDocument& toJson();

    // Abonnement à une clé (les callbacks recevront la nouvelle valeur en paramètre)
    void subscribe(const String& key, std::function<void(JsonVariant)> callback);

private:
    DataStore();
    // Document JSON pour stocker les données (utilisation d'ArduinoJson 7, dynamique)
    JsonDocument doc;

    // Stockage des callbacks : plusieurs callbacks par clé
    std::map<String, std::vector<std::function<void(JsonVariant)>>> callbacks;
};

// --- Implémentation des méthodes template (dans le header) ---

template <typename T>
T DataStore::get(const String& key, T defaultValue) {
    // Utiliser directement doc[key].is<JsonObject>() au lieu de containsKey()
    if (doc[key].is<JsonObject>()) {
        JsonObject entry = doc[key].as<JsonObject>();
        // Vérifier si "value" existe
        if (!entry["value"].isNull()) {
            return entry["value"].as<T>();
        }
    }
    return defaultValue;
}

template <typename T>
T DataStore::getWithTypeCheck(const String& key, T defaultValue) {
    if (!exists(key)) {
        Serial.printf("[DATASTORE] La clé '%s' n'existe pas, retour de la valeur par défaut\n", key.c_str());
        return defaultValue;
    }
    
    if (!isOfType<T>(key)) {
        Serial.printf("[DATASTORE] Type incorrect pour la clé '%s', attendu: %s, trouvé: %s\n", 
                     key.c_str(), 
                     dataTypeToString(getDataType<T>()).c_str(), 
                     dataTypeToString(getValueType(key)).c_str());
        return defaultValue;
    }
    
    return get<T>(key, defaultValue);
}

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
    entry["type"] = dataTypeToString(getDataType<T>()); // Utiliser la nouvelle méthode
    entry["tricks"] = millis();  // Stocke le timestamp en ticks

    // Exécution des callbacks abonnés (s'ils existent)
    if (callbacks.find(key) != callbacks.end()) {
        Serial.printf("[DATASTORE] Changement détecté sur '%s', exécution des callbacks\n", key.c_str());
        for (auto& cb : callbacks[key]) {
            cb(entry["value"]);  // Transmission de la nouvelle valeur
        }
    }
}

#endif // DATASTORE_H