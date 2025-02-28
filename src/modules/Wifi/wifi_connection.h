
#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

/************************************
 *  FONCTION : initializeWifi
 ************************************/

void initializeWiFi(String wifiSSID, String wifiPassword) {
    WiFiManager wifiManager;
    const int maxRetries = 3;
    int retryCount = 0;
    bool connected = false;

    while (retryCount < maxRetries && !connected) {
        if (wifiManager.autoConnect("AutoConnectAP", "password")) {
            Serial.println("Connexion Wi-Fi réussie !");
            connected = true;
        } else {
            retryCount++;
            Serial.println("Échec de la connexion Wi-Fi, nouvelle tentative...");
        }
    }

    if (!connected) {
        Serial.println("Impossible de se connecter au Wi-Fi après plusieurs tentatives. Démarrage en mode autonome.");
        // Placez ici le code à exécuter lorsque le Wi-Fi n'est pas disponible
    }
}