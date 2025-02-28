#ifndef DATASTORE_H
#define DATASTORE_H

#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <array>
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

// --- Classe de journalisation pour le DataStore ---
class DataStoreLogger {
private:
    static const int MAX_ERRORS = 10;
    std::array<String, MAX_ERRORS> errorLog;
    int errorIndex = 0;
    bool hasWrapped = false;

public:
    void logError(const String& message) {
        // Horodatage
        String timestamp = String(millis());
        String logEntry = "[" + timestamp + "] " + message;
        
        // Stocker l'erreur
        errorLog[errorIndex] = logEntry;
        
        // Afficher sur la console série (pour débogage immédiat)
        Serial.println("ERREUR DATASTORE: " + logEntry);
        
        // Mettre à jour l'index pour la prochaine erreur
        errorIndex = (errorIndex + 1) % MAX_ERRORS;
        if (errorIndex == 0) {
            hasWrapped = true;
        }
    }
    
    void printErrors() {
        Serial.println("=== JOURNAL DES ERREURS DATASTORE ===");
        
        int start = hasWrapped ? errorIndex : 0;
        int count = hasWrapped ? MAX_ERRORS : errorIndex;
        
        for (int i = 0; i < count; i++) {
            int index = (start + i) % MAX_ERRORS;
            Serial.println(errorLog[index]);
        }
        
        Serial.println("=====================================");
    }
    
    void clearErrors() {
        errorIndex = 0;
        hasWrapped = false;
        for (int i = 0; i < MAX_ERRORS; i++) {
            errorLog[i] = "";
        }
    }
};

// --- Déclaration de la classe DataStore ---
class DataStore {
public:
    static DataStore& getInstance();  // Singleton

    // Vérifier si une clé existe dans le store
    bool exists(const String& key);

    // MÉTHODE UNIQUE: Méthode get avec vérification d'erreur et indication du succès/échec
    template <typename T>
    bool get(const String& key, T& outValue);

    // Setter générique : encapsule la donnée dans un objet JSON
    template <typename T>
    void set(const String& key, T value);

    // Récupère l'objet complet associé à la clé
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
    
    // Méthode pour vérifier si une donnée est obsolète
    bool isStale(const String& key, unsigned long maxAgeTicks);

    // Affichage du contenu du DataStore
    void printStore();

    // Gestion des erreurs
    void printErrors();
    void clearErrors();

    // Conversion en JSON pour MQTT et affichage
    JsonDocument& toJson();

    // Abonnement à une clé (les callbacks recevront la nouvelle valeur en paramètre)
    void subscribe(const String& key, std::function<void(JsonVariant)> callback);

private:
    DataStore();
    // Document JSON pour stocker les données
    JsonDocument doc;
    
    // Logger pour les erreurs
    DataStoreLogger logger;

    // Stockage des callbacks : plusieurs callbacks par clé
    std::map<String, std::vector<std::function<void(JsonVariant)>>> callbacks;
    
    // Méthode privée pour journaliser les erreurs
    void logError(const String& message);
};

// --- Implémentation des méthodes template (dans le header) ---

// MÉTHODE UNIQUE: Méthode get avec vérification d'erreur
template <typename T>
bool DataStore::get(const String& key, T& outValue) {
    // Vérifier si la clé existe
    if (!exists(key)) {
        logError("Clé non trouvée: " + key);
        return false;
    }
    
    // Vérifier si le type est correct
    if (!isOfType<T>(key)) {
        logError("Type incorrect pour la clé: " + key + ", attendu: " + 
                dataTypeToString(getDataType<T>()) + ", trouvé: " + 
                dataTypeToString(getValueType(key)));
        return false;
    }
    
    // Récupérer la valeur
    JsonObject entry = doc[key].as<JsonObject>();
    outValue = entry["value"].as<T>();
    return true;
}

template <typename T>
void DataStore::set(const String& key, T value) {
    JsonObject entry;
    // Créer ou récupérer l'objet associé à la clé
    if (doc[key].is<JsonObject>()) {
        entry = doc[key].as<JsonObject>();
    } else {
        // Créer un nouvel objet JSON
        entry = doc[key].to<JsonObject>();
    }

    // Vérifier si "value" est déjà présente et égale à la nouvelle valeur
    if (!entry["value"].isNull() && entry["value"] == value) {
        return;
    }

    // Mise à jour de l'entrée
    entry["value"] = value;
    entry["type"] = dataTypeToString(getDataType<T>());
    entry["tricks"] = millis();  // Stocke le timestamp en ticks

    // Exécution des callbacks abonnés (s'ils existent)
    if (callbacks.find(key) != callbacks.end()) {
        Serial.printf("[DATASTORE] Changement détecté sur '%s', exécution des callbacks\n", key.c_str());
        for (auto& cb : callbacks[key]) {
            cb(entry["value"]);  // Transmission de la nouvelle valeur
        }
    }
}

// Implémentation de logError
inline void DataStore::logError(const String& message) {
    logger.logError(message);
}

// Implémentation des méthodes de journalisation
inline void DataStore::printErrors() {
    logger.printErrors();
}

inline void DataStore::clearErrors() {
    logger.clearErrors();
}

#endif // DATASTORE_H