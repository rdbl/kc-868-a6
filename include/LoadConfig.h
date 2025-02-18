#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H

#include <ArduinoJson.h>

// Structure pour une entité (device, actuator, sensor)
struct Entity {
    String id;
    String name;
    String model;
    String description;
    int pin ;
    String room;
    String datasJson; // Stocke dynamiquement les données sous forme de JSON string
};

// Définition du nombre max d'actuators et sensors
#define MAX_ACTUATORS 10
#define MAX_SENSORS 10

// Structure principale pour stocker toute la configuration
struct Config {
    // WiFi
    String wifiSSID;
    String wifiPassword;

    // MQTT
    String mqttHost;
    uint16_t mqttPort;
    String mqttUser;
    String mqttPassword;
    String mqttTopic;

    // Device
    Entity device;
    Entity actuators[MAX_ACTUATORS];
    int actuatorsCount;

    Entity sensors[MAX_SENSORS];
    int sensorsCount;
};

// Déclaration globale pour accéder à la configuration dans `main.cpp`
extern Config config;

// Fonctions pour charger la configuration
void loadConfig();
void printConfig(Config config);

#endif
