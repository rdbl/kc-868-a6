#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H

#include <ArduinoJson.h>

// Structure pour le header d'une entité
struct Header {
  String name;
  String model;
  String description;
  String room;  // Ce champ est utilisé principalement pour le device principal, il pourra être laissé vide pour d'autres entités.
};

// Structure pour la configuration WiFi
struct WifiConfig {
  String ssid;
  String password;
};

// Structure pour la configuration MQTT
struct MqttConfig {
  String host;
  uint16_t port;
  String user;
  String password;
  String topic;
};

// Structure pour la configuration OTA
struct OtaConfig {
  bool enabled;
  uint16_t port;
};

// Structure générique pour une entité (device, actuator, sensor, display)
struct Entity {
  String id;
  Header header;          // Contient name, model, description, et éventuellement room
  String hardwareJson;    // Stocke les données hardware (ex : pins, bus I2C, etc.) au format JSON
  String dataJson;        // Stocke les données opérationnelles spécifiques (états, délais, etc.) au format JSON
};

// Définition du nombre maximum d'entités par catégorie
#define MAX_DISPLAYS  10
#define MAX_ACTUATORS 10
#define MAX_SENSORS   10

// Structure principale pour stocker toute la configuration
struct Config {
  bool simulationMode;

  WifiConfig wifi;
  MqttConfig mqtt;
  OtaConfig ota;
  
  // Configuration du device principal
  Entity device;

  // Liste des displays associés au device
  Entity displays[MAX_DISPLAYS];
  int displaysCount;
  
  // Liste des actuators
  Entity actuators[MAX_ACTUATORS];
  int actuatorsCount;
  
  // Liste des sensors
  Entity sensors[MAX_SENSORS];
  int sensorsCount;
};

// Déclaration globale pour accéder à la configuration dans main.cpp
extern Config config;

// Fonctions pour charger et afficher la configuration
String correctHexInJson(String content);
void loadConfig();
void printConfig(const Config &config);

#endif
