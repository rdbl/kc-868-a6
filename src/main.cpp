#include "EventManager.h"
#include "HardwareManager.h"
#include "DataStore.h"
#include <Arduino.h>

// Variables globales pour le débogage
float capteur1_temperature = -100.0f;  // Valeur impossible pour détecter si elle change
float capteur2_temperature = -100.0f;  // Valeur impossible pour détecter si elle change

/**
 * @brief Configuration initiale du système minimal
 */
void setup() {
    // Initialisation de la liaison série pour le débogage
    Serial.begin(115200);
    delay(1000);  // Attendre que le port série soit prêt
    
    Serial.println("\n\n[DÉMARRAGE] Système minimal de lecture des capteurs");
    Serial.println("===========================================");
    
    // Obtenir les références aux singletons
    EventManager& eventManager = EventManager::getInstance();
    DataStore& dataStore = DataStore::getInstance();
    
    // S'abonner aux événements des capteurs de température
    // Cette fonction sera appelée quand le capteur 1 enverra une lecture de température
    eventManager.subscribe<float>("Sensor.0.Temperature", [](float value) {
        Serial.printf("[CAPTEUR-1] Nouvelle température : %.2f°C\n", value);
        capteur1_temperature = value;  // Stockage dans une variable globale pour vérification
    });
    
    // Cette fonction sera appelée quand le capteur 2 enverra une lecture de température
    eventManager.subscribe<float>("Sensor.1.Temperature", [](float value) {
        Serial.printf("[CAPTEUR-2] Nouvelle température : %.2f°C\n", value);
        capteur2_temperature = value;  // Stockage dans une variable globale pour vérification
    });
    
    // Initialiser le matériel (notamment les capteurs DS18B20)
    Serial.println("[SYSTÈME] Initialisation du matériel...");
    HardwareManager::getInstance().initialize();
    Serial.println("[SYSTÈME] Matériel initialisé.");
    
    // Planifier une lecture des capteurs toutes les 5 secondes
    // ⚠️ Ne pas oublier le cast explicite pour éviter les ambiguïtés
    eventManager.schedule("UpdateSensors", static_cast<unsigned long>(5000), 0);
    
    Serial.println("[SYSTÈME] Démarrage des lectures périodiques (toutes les 5 secondes).");
    Serial.println("[SYSTÈME] Initialisation terminée.");
    Serial.println("===========================================");
}

/**
 * @brief Boucle principale - simplifiée au maximum
 */
void loop() {
    // Traiter les événements planifiés (notamment UpdateSensors)
    EventManager::getInstance().processScheduledEvents();
    
    // Vérifier périodiquement si les valeurs dans le DataStore correspondent
    // aux valeurs globales récupérées par les callbacks
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {  // Vérification toutes les 10 secondes
        lastCheck = millis();
        
        DataStore& ds = DataStore::getInstance();
        float temp1_datastore = 0.0f;
        float temp2_datastore = 0.0f;
        
        // Essai de lecture avec la méthode get classique
        bool success1= ds.get("Sensor.0.Temperature", temp1_datastore);
        bool success2 = ds.get("Sensor.1.Temperature", temp2_datastore);
        
        // Affichage des résultats pour diagnostic
        Serial.println("\n--- DIAGNOSTIC DES CAPTEURS ---");
        
        // Variables globales (valeurs reçues directement des événements)
        Serial.printf("Capteur 1 - Valeur globale    : %.2f°C\n", capteur1_temperature);
        Serial.printf("Capteur 2 - Valeur globale    : %.2f°C\n", capteur2_temperature);
        
        // Valeurs dans le DataStore via tryGet
        Serial.printf("Capteur 1 - get DataStore  : %s (%.2f°C)\n", 
                     success1 ? "OK" : "ÉCHEC", temp1_datastore);
        Serial.printf("Capteur 2 - get DataStore  : %s (%.2f°C)\n", 
                     success2 ? "OK" : "ÉCHEC", temp2_datastore);
                
        // Vérifier si la clé existe dans le DataStore
        Serial.printf("Capteur 1 - La clé existe     : %s\n", 
                     ds.exists("Sensor.0.Temperature") ? "OUI" : "NON");
        Serial.printf("Capteur 2 - La clé existe     : %s\n", 
                     ds.exists("Sensor.1.Temperature") ? "OUI" : "NON");
                     
        // Lister toutes les clés du DataStore pour vérifier
        Serial.println("\nListe de toutes les clés dans le DataStore :");
        ds.printStore();
        
        Serial.println("--- FIN DU DIAGNOSTIC ---\n");
    }
    
    // Petite pause pour éviter de surcharger le processeur
    delay(10);
}