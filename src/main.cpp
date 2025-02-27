#include "EventManager.h"
#include "HardwareManager.h"


void setup() {
    Serial.begin(115200);

    // 🔹 Initialiser le matériel
    HardwareManager::getInstance().initialize();

    // 🔹 Abonnement à `TemperatureSensor1` et `TemperatureSensor2`
    EventManager::getInstance().subscribe("TemperatureSensor1", [](float value) {
        Serial.printf("[EVENT] Température capteur 1 : %.2f°C\n", value);
    });

    EventManager::getInstance().subscribe("TemperatureSensor2", [](float value) {
        Serial.printf("[EVENT] Température capteur 2 : %.2f°C\n", value);
    });

    // 🔹 Planifier une mise à jour toutes les 10 secondes
    // EventManager::getInstance().schedule("UpdateSensors", 0.0, 10000, 5);

    // 🔹 Allumer le relais après 5 secondes
    EventManager::getInstance().schedule("RelayControl", HIGH, 5000, 10);

    // 🔹 Éteindre le relais après 10 secondes
    EventManager::getInstance().schedule("RelayControl", LOW, 10000, 10);

    // 🔹 Allumer le relais après 5 secondes
    EventManager::getInstance().schedule("RelayControl", HIGH, 50000, 10);
}

void loop() {
    EventManager::getInstance().processScheduledEvents();
    
    // Afficher la liste des événements en attente toutes les 2 secondes, par exemple
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
        lastPrint = millis();
        EventManager::getInstance().printScheduledEvents();
        EventManager::getInstance().publish("UpdateSensors", true);
    }
    
}


