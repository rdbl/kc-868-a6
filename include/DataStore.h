#ifndef DATASTORE_H
#define DATASTORE_H

#include <ArduinoJson.h>
#include <map>
#include <functional>

class DataStore {
public:
    static DataStore& getInstance();  // Singleton

    // Getter générique
    template <typename T>
    T get(const String& key, T defaultValue = T());

    // Setter générique avec activation des callbacks
    template <typename T>
    void set(const String& key, T value);

    // Chargement de `configuration.yaml` dans le DataStore
    void loadFromConfig();

    // Affichage du contenu du DataStore
    void printStore();

    // Convertir en JSON pour MQTT et affichage
    JsonDocument& toJson();

    // 🔹 Abonnement à une clé pour déclencher un callback
    void subscribe(const String& key, std::function<void()> callback);

private:
    DataStore();
    JsonDocument doc;

    // 🔹 Stockage des callbacks
    std::map<String, std::function<void()>> callbacks;
};

#endif
