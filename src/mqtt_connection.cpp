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
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Default callback invoked.");
    Serial.print("[MQTT]-[mqqt_connection.cpp]- Topic: ");
    Serial.println(topic);

    // Construire le payload en tant que chaîne
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("[MQTT]-[mqqt_connection.cpp]- Payload: ");
    Serial.println(message);
}

void setMqttCallback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    customMqttCallback = callback;
}


// ===================================
// Connexion au broker
// ===================================

static unsigned long lastMqttAttempt = 0;
static const unsigned long mqttRetryInterval = 5000; // 5 secondes

static void connectMqttBroker(const char* user, const char* pass) {
  // Si déjà connecté, ne rien faire
  if (mqttClient.connected()) return;
  
  unsigned long now = millis();
  if (now - lastMqttAttempt < mqttRetryInterval) {
    // Pas encore le moment de réessayer
    return;
  }
  
  lastMqttAttempt = now;
  
  Serial.print("[MQTT]-[mqqt_connection.cpp]- Attempting MQTT connection...");
  String clientId = "ESP32-";
  clientId += String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str(), user, pass)) {
    Serial.println(" connected!");

    // Abonnements
    mqttClient.subscribe("Chauffage/bouilleur_dev/homeassistant/actuators/0/params/state");
    mqttClient.subscribe("Chauffage/bouilleur_dev/homeassistant/actuators/0/params/mode");
    mqttClient.subscribe("Chauffage/bouilleur_dev/homeassistant/actuators/0/params/diff_start");
    mqttClient.subscribe("Chauffage/bouilleur_dev/homeassistant/actuators/0/params/diff_stop");
    mqttClient.subscribe("Chauffage/bouilleur_dev/homeassistant/actuators/0/params/relay_delay");
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Subscribed to topics.");
  } else {
    Serial.print("[ERROR]--[mqqt_connection.cpp]- failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" -> will retry soon.");
  }
}


// ===================================
// Fonctions exposées
// ===================================
void initMqtt(String mqttBroker, int mqttPort, String mqttUser, String mqttPass) {
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);

    // Utiliser le callback utilisateur si défini, sinon utiliser le callback par défaut
    mqttClient.setCallback(customMqttCallback ? customMqttCallback : defaultMqttCallback);

    // Sauvegarde pour la reconnexion
    g_mqttUser = mqttUser.c_str();
    g_mqttPass = mqttPass.c_str();

    // Connexion initiale
    connectMqttBroker(g_mqttUser, g_mqttPass);
    mqttClient.setBufferSize(1024);
}

void handleMqtt() {
  // Si on est déconnecté, on tente de se reconnecter
  if (!mqttClient.connected()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Disconnected, attempting to reconnect...");
    connectMqttBroker(g_mqttUser, g_mqttPass);
  }
  mqttClient.loop();
}

// Validation JSON, unchanged
static bool validateJsonStructure(const JsonDocument& doc) {
  if (!doc["id"].is<String>()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Validation error: missing 'id'");
    Serial.println(doc["id"].as<String>());
    return false;
  }
  if (!doc["header"].is<JsonVariantConst>()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Validation error: missing 'header'");
    Serial.println(doc["header"].as<String>());
    return false;
  }
  if (!doc["extra"].is<JsonVariantConst>()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Validation error: missing 'extra'");
    Serial.println(doc["extra"].as<String>());
    return false;
  }
  if (!doc["sensors"].is<JsonVariantConst>()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Validation error: missing 'sensors'");
    Serial.println(doc["sensors"].as<String>());
    return false;
  }
  if (!doc["actuators"].is<JsonVariantConst>()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Validation error: missing 'actuators'");
    Serial.println(doc["actuators"].as<String>());
    return false;
  }
  return true;
}

bool publishAllData(String topic, const JsonDocument& doc) {
  // 1) Contrôle de conformité
  if (!validateJsonStructure(doc)) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- JSON document is not valid -> not published");
    return false;
  }

  // 2) Vérifier la connexion (optionnel, handleMqtt() va le faire, mais bon)
  if (!mqttClient.connected()) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Client not connected -> not published");
    return false;
  }

  // 3) Sérialiser dans un buffer
  static char buffer[1024];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  Serial.print("[MQTT]-[mqqt_connection.cpp]- Serialized JSON length: ");
  Serial.println(len);

  if (len >= sizeof(buffer)) {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- JSON message too large -> not published");
    return false;
  }

  // 4) Publication
  bool success = mqttClient.publish(topic.c_str(), buffer, len);
  if (success) {
    Serial.print("[MQTT]-[mqqt_connection.cpp]- Message published successfully to topic ");
    Serial.println(topic);
  } else {
    Serial.println("[MQTT]-[mqqt_connection.cpp]- Failed to publish message");
  }

  return success;
}
