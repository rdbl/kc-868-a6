#include "Wifi_module.h"

// Implémentation du singleton
Wifi_module& Wifi_module::getInstance() {
    static Wifi_module instance;
    return instance;
}

// Constructeur
Wifi_module::Wifi_module() 
    : initialized_(false), 
      connected_(false),
      ssid_(""),
      password_(""),
      ip_address_(""),
      lastStatusCheck_(0) {
}

// Initialisation du module
void Wifi_module::initialize() {
    if (initialized_) return;

    Serial.println("[WIFI] Initialisation du module WiFi");
    
    // S'abonner à l'événement de disponibilité de la configuration WiFi
    eventManager_.subscribe("WiFiConfigAvailable", [this]() {
        this->onWiFiConfigAvailable();
    });
    
    // S'abonner à l'événement de redémarrage du WiFi
    eventManager_.subscribe("WiFiRestart", [this]() {
        this->disconnect();
        this->connect();
    });
    
    // Vérifier si la configuration est déjà disponible dans le DataStore
    if (dataStore_.exists("WiFi.SSID") && dataStore_.exists("WiFi.Password")) {
        onWiFiConfigAvailable();
    }
    
    initialized_ = true;
    Serial.println("[WIFI] Module WiFi initialisé");
}

// Méthode loop pour la vérification périodique
void Wifi_module::loop() {
    // Vérifier l'état de la connexion toutes les 30 secondes
    unsigned long now = millis();
    if (now - lastStatusCheck_ >= 30000) {
        lastStatusCheck_ = now;
        
        // Vérifier si le WiFi est toujours connecté
        if (!isConnected()) {
            Serial.println("[WIFI] Connexion perdue, tentative de reconnexion...");
            connect();
        }
    }
}

// Traitement de l'événement de disponibilité de configuration WiFi
void Wifi_module::onWiFiConfigAvailable() {
    Serial.println("[WIFI] Configuration WiFi disponible, chargement des paramètres");
    
    // Charger les paramètres WiFi du DataStore
    if (!dataStore_.get("WiFi.SSID", ssid_)) {
        Serial.println("[WIFI] ERREUR: Impossible de lire le SSID WiFi");
        return;
    }
    
    if (!dataStore_.get("WiFi.Password", password_)) {
        Serial.println("[WIFI] ERREUR: Impossible de lire le mot de passe WiFi");
        return;
    }
    
    Serial.printf("[WIFI] Paramètres chargés - SSID: %s\n", ssid_.c_str());
    
    // Se connecter au WiFi avec les nouveaux paramètres
    connect();
}

// Connexion au WiFi
void Wifi_module::connect() {
    if (ssid_.isEmpty()) {
        Serial.println("[WIFI] ERREUR: SSID vide, impossible de se connecter");
        // Utiliser WiFiManager pour configurer le WiFi en mode AP
        WiFiManager wifiManager;
        wifiManager.autoConnect("InsertBouilleur-AP", "password");
        return;
    }
    
    Serial.printf("[WIFI] Tentative de connexion au réseau: %s\n", ssid_.c_str());
    
    // Déconnecter d'abord si déjà connecté
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(500);
    }
    
    // Démarrer la connexion
    WiFi.begin(ssid_.c_str(), password_.c_str());
    
    // Attendre la connexion (avec timeout)
    int attempt = 0;
    const int max_attempts = 20; // 10 secondes de timeout
    
    while (WiFi.status() != WL_CONNECTED && attempt < max_attempts) {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        connected_ = true;
        ip_address_ = WiFi.localIP().toString();
        Serial.printf("\n[WIFI] Connecté! Adresse IP: %s\n", ip_address_.c_str());
        
        // Publier l'événement de connexion WiFi
        eventManager_.publish("WiFiConnected");
        
        // Stocker les informations de connexion dans le DataStore
        dataStore_.set("WiFi.Status", "connected");
        dataStore_.set("WiFi.IP", ip_address_);
    } else {
        connected_ = false;
        Serial.println("\n[WIFI] Échec de la connexion après timeout");
        
        // Publier l'événement d'échec de connexion WiFi
        eventManager_.publish("WiFiConnectionFailed");
        
        // Stocker les informations d'échec dans le DataStore
        dataStore_.set("WiFi.Status", "disconnected");
        
        // Démarrer WiFiManager en mode AP comme fallback
        Serial.println("[WIFI] Démarrage du mode point d'accès pour configuration");
        WiFiManager wifiManager;
        bool success = wifiManager.autoConnect("InsertBouilleur-AP", "password");
        
        if (success) {
            connected_ = true;
            ip_address_ = WiFi.localIP().toString();
            Serial.printf("[WIFI] Connecté via WiFiManager! Adresse IP: %s\n", ip_address_.c_str());
            
            // Sauvegarder les nouveaux paramètres WiFi
            ssid_ = WiFi.SSID();
            password_ = WiFi.psk();
            
            // Mettre à jour le DataStore
            dataStore_.set("WiFi.SSID", ssid_);
            dataStore_.set("WiFi.Password", password_);
            dataStore_.set("WiFi.Status", "connected");
            dataStore_.set("WiFi.IP", ip_address_);
            
            // Publier l'événement de connexion WiFi
            eventManager_.publish("WiFiConnected");
        } else {
            Serial.println("[WIFI] Échec de configuration via WiFiManager");
            dataStore_.set("WiFi.Status", "ap_failed");
        }
    }
}

// Déconnexion du WiFi
void Wifi_module::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WIFI] Déconnexion du réseau WiFi");
        WiFi.disconnect();
        connected_ = false;
        ip_address_ = "";
        
        // Mettre à jour le DataStore
        dataStore_.set("WiFi.Status", "disconnected");
        
        // Publier l'événement de déconnexion WiFi
        eventManager_.publish("WiFiDisconnected");
    }
}

// Vérifier si le WiFi est connecté
bool Wifi_module::isConnected() {
    connected_ = (WiFi.status() == WL_CONNECTED);
    return connected_;
}

// Obtenir l'adresse IP
String Wifi_module::getIPAddress() {
    if (isConnected()) {
        ip_address_ = WiFi.localIP().toString();
        return ip_address_;
    }
    return "";
}