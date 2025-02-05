#include "mqtt_connection.h"
#include <WiFi.h>         
#include <PubSubClient.h>

// ===================================
// Variables statiques
// ===================================
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// On stocke les credentials pour la reconnexion
static const char* g_mqttUser = nullptr;
static const char* g_mqttPass = nullptr;

// ===================================
// Callback lorsque le MQTT reçoit un message
// ===================================// Définir dynamiquement le callback
static void (*customMqttCallback)(char* topic, byte* payload, unsigned int length) = nullptr;

// Callback par défaut
// Callback par défaut
static void defaultMqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("[MQTT] Default callback invoked.");
    Serial.print("[MQTT] Topic: ");
    Serial.println(topic);

    // Construire le payload en tant que chaîne
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("[MQTT] Payload: ");
    Serial.println(message);
}

void setMqttCallback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    customMqttCallback = callback;
}


// ===================================
// Connexion au broker
// ===================================
static void connectMqttBroker(const char* user, const char* pass) {
    while (!mqttClient.connected()) {
        Serial.print("[MQTT] Attempting MQTT connection...");
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str(), user, pass)) {
            Serial.println(" connected!");

            // Ajouter les abonnements nécessaires ici
            mqttClient.subscribe("Chauffage/bouilleur/homeassistant/actuators/0/params/state");
            mqttClient.subscribe("Chauffage/bouilleur/homeassistant/actuators/0/params/mode");
            mqttClient.subscribe("Chauffage/bouilleur/homeassistant/actuators/0/params/diff_temp");

            Serial.println("[MQTT] Subscribed to topics.");
        } else {
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" -> retry in 5 seconds");
            delay(5000);
        }
    }
}

// ===================================
// Fonctions exposées
// ===================================
void initMqtt(const char* mqttBroker, int mqttPort, const char* mqttUser, const char* mqttPass) {
    mqttClient.setServer(mqttBroker, mqttPort);

    // Utiliser le callback utilisateur si défini, sinon utiliser le callback par défaut
    mqttClient.setCallback(customMqttCallback ? customMqttCallback : defaultMqttCallback);

    // Sauvegarde pour la reconnexion
    g_mqttUser = mqttUser;
    g_mqttPass = mqttPass;

    // Connexion initiale
    connectMqttBroker(g_mqttUser, g_mqttPass);
    mqttClient.setBufferSize(1024);
}

void handleMqtt() {
  // Si on est déconnecté, on tente de se reconnecter
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Disconnected, attempting to reconnect...");
    connectMqttBroker(g_mqttUser, g_mqttPass);
  }
  mqttClient.loop();
}

// Validation JSON, unchanged
static bool validateJsonStructure(const JsonDocument& doc) {
  if (!doc["id"].is<String>()) {
    Serial.println("[MQTT] Validation error: missing 'id'");
    Serial.println(doc["id"].as<String>());
    return false;
  }
  if (!doc["header"].is<JsonVariantConst>()) {
    Serial.println("[MQTT] Validation error: missing 'header'");
    Serial.println(doc["header"].as<String>());
    return false;
  }
  if (!doc["sensors"].is<JsonVariantConst>()) {
    Serial.println("[MQTT] Validation error: missing 'sensors'");
    Serial.println(doc["sensors"].as<String>());
    return false;
  }
  if (!doc["actuators"].is<JsonVariantConst>()) {
    Serial.println("[MQTT] Validation error: missing 'actuators'");
    Serial.println(doc["actuators"].as<String>());
    return false;
  }
  return true;
}

bool publishAllData(const char* topic, const JsonDocument& doc) {
  // 1) Contrôle de conformité
  if (!validateJsonStructure(doc)) {
    Serial.println("[MQTT] JSON document is not valid -> not published");
    return false;
  }

  // 2) Vérifier la connexion (optionnel, handleMqtt() va le faire, mais bon)
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Client not connected -> not published");
    return false;
  }

  // 3) Sérialiser dans un buffer
  static char buffer[512];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  Serial.print("[MQTT] Serialized JSON length: ");
  Serial.println(len);

  if (len >= sizeof(buffer)) {
    Serial.println("[MQTT] JSON message too large -> not published");
    return false;
  }

  // 4) Publication
  bool success = mqttClient.publish(topic, buffer, len);
  if (success) {
    Serial.print("[MQTT] Message published successfully to topic ");
    Serial.println(topic);
  } else {
    Serial.println("[MQTT] Failed to publish message");
  }

  return success;
}
