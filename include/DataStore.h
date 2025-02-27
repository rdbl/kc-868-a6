#ifndef DATASTORE_H
#define DATASTORE_H

#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <functional>

// --- Fonctions helper pour obtenir le nom du type ---
inline String getTypeName(int) { return "int"; }
inline String getTypeName(unsigned int) { return "unsigned int"; }
inline String getTypeName(long) { return "long"; }
inline String getTypeName(unsigned long) { return "unsigned long"; }
inline String getTypeName(float) { return "float"; }
inline String getTypeName(double) { return "double"; }
inline String getTypeName(bool) { return "bool"; }
inline String getTypeName(const String&) { return "String"; }
inline String getTypeName(const char*) { return "const char*"; }
template <typename T>
inline String getTypeName(T) { return "unknown"; }

// --- Déclaration de la classe DataStore ---
class DataStore {
public:
    static DataStore& getInstance();  // Singleton

    // Getter générique : retourne la valeur stockée (extraite du champ "value")
    template <typename T>
    T get(const String& key, T defaultValue = T());

    // Setter générique : encapsule la donnée dans un objet JSON contenant "value", "type" et "tricks"
    template <typename T>
    void set(const String& key, T value);

    // Récupère l'objet complet associé à la clé (contenant "value", "type" et "tricks")
    JsonObject getObject(const String& key);

    // Chargement de la configuration externe (implémentation ultérieure)
    void loadFromConfig();

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

#endif // DATASTORE_H
