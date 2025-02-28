#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "Module.h"

class Wifi_module : public Module {
public:
    // Singleton
    static Wifi_module& getInstance();

    // Implémentation de l'interface Module
    void initialize() override;
    bool isInitialized() const override { return initialized_; }
    String getName() const override { return "WiFi"; }
    void loop() override; // Pour vérifier périodiquement l'état de la connexion

    // Gestion du WiFi
    void connect();
    void disconnect();
    bool isConnected();
    String getIPAddress();

private:
    Wifi_module();
    
    // Callback pour les événements
    void onWiFiConfigAvailable();
    
    // Variables membres
    bool initialized_;
    bool connected_;
    String ssid_;
    String password_;
    String ip_address_;
    unsigned long lastStatusCheck_;
};

#endif // WIFI_MODULE_H