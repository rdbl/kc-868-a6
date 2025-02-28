#ifndef MQTT_CONNECTION_H
#define MQTT_CONNECTION_H

#include <ArduinoJson.h>

// Initialise la connexion MQTT (à appeler dans le setup)
void initMqtt(String mqttBroker, int mqttPort, String mqttUser, String mqttPass);

// À appeler régulièrement dans la loop pour maintenir la connexion MQTT
void handleMqtt();

/**
 * Publie un document JSON déjà construit par l'utilisateur.
 * @param topic : le topic MQTT dans lequel publier
 * @param doc   : le JsonDocument à publier
 * @return true si publication réussie et structure validée, false sinon
 */
bool publishAllData(String topic, const JsonDocument& doc);

/**
 * Permet de définir dynamiquement le callback MQTT.
 * @param callback : Pointeur vers la fonction callback à utiliser
 */
void setMqttCallback(void (*callback)(char* topic, byte* payload, unsigned int length));


#endif
