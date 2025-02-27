#include "EventManager.h"
#include "HardwareManager.h"


void setup() {
    Serial.begin(115200);
    Serial.println("[MAIN] Démarrage du système de régulation bouilleur");

    // 🔹 Initialiser le matériel
    HardwareManager::getInstance().initialize();

    // 🔹 Abonnement à `TemperatureSensor1` et `TemperatureSensor2`
    // Utilisation de la nouvelle syntaxe avec typage explicite <float>
    EventManager::getInstance().subscribe<float>("TemperatureSensor1", [](float value) {
        Serial.printf("[EVENT] Température capteur 1 : %.2f°C\n", value);
    });

    EventManager::getInstance().subscribe<float>("TemperatureSensor2", [](float value) {
        Serial.printf("[EVENT] Température capteur 2 : %.2f°C\n", value);
    });

    // 🔹 Nouvel abonnement sans valeur pour démontrer la fonctionnalité
    EventManager::getInstance().subscribe("SystemStarted", []() {
        Serial.println("[EVENT] Le système a démarré avec succès !");
    });

    // 🔹 Planifier des événements
    
    // Publication immédiate d'un événement sans valeur
    EventManager::getInstance().publish("SystemStarted");

    // 🔹 Planifier une mise à jour sans valeur toutes les 10 secondes (nouvelle syntaxe)
    // EventManager::getInstance().schedule("UpdateSensors", 10000, 5);

    // 🔹 Allumer le relais après 5 secondes (même syntaxe qu'avant)
    EventManager::getInstance().schedule("RelayControl", true, 5000, 10);

    // 🔹 Éteindre le relais après 10 secondes (même syntaxe qu'avant)
    EventManager::getInstance().schedule("RelayControl", false, 10000, 10);

    // 🔹 Allumer le relais après 15 secondes (même syntaxe qu'avant)
    EventManager::getInstance().schedule("RelayControl", true, 15000, 10);
    
    Serial.println("[MAIN] Configuration terminée");
}

void loop() {
    // Traiter les événements planifiés
    EventManager::getInstance().processScheduledEvents();
    
    // Afficher la liste des événements en attente toutes les 2 secondes
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
        lastPrint = millis();
        EventManager::getInstance().printScheduledEvents();
        
        // Utiliser la nouvelle syntaxe sans valeur pour UpdateSensors
        EventManager::getInstance().publish("UpdateSensors");
    }
}