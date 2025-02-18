#include "display_connection.h"
#include <Arduino.h>




// Constructeur : n'appelle pas u8g2.begin() immédiatement
DisplayConnection::DisplayConnection() : u8g2(U8G2_R2, 15, 4, U8X8_PIN_NONE) {
    // Ne fait rien ici, on initialisera plus tard avec begin()
}

// Nouvelle fonction pour initialiser l'OLED
void DisplayConnection::begin() {
    Serial.println("Scanning I2C before initializing OLED...");
    Serial.println("Initializing OLED...");
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    Serial.println("OLED initialized!");
    oled_initialized_ = true;  // ✅ Marque que l'écran est bien initialisé

}

// Fonction d'affichage
void DisplayConnection::displayInfos(float temp1, float temp2, float tempDifference, 
                                     bool relayState, int countdown, 
                                     bool isCountdownActive, bool isAutoMode) {
    
    if (!oled_initialized_) {
        Serial.println("[INFO] OLED not initialized, skipping display update.");
        return;  // ✅ On sort si l'écran n'a pas été détecté
    }

    
    if (!isCountdownActive) countdown = 0;

    Serial.printf("Temp S1: %.2f°C | Temp S2: %.2f°C | Diff: %.2f°C\n", temp1, temp2, tempDifference);
    Serial.printf("Relay State: %s | Countdown: %d s\n", relayState ? "ON" : "OFF", countdown);

    // Affichage OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "S1(bleu): %.2f°C", temp1);
    u8g2.drawStr(0, 10, buffer);

    snprintf(buffer, sizeof(buffer), "S2(blanc): %.2f°C", temp2);
    u8g2.drawStr(0, 22, buffer);

    snprintf(buffer, sizeof(buffer), "Diff: %.2f°C", tempDifference);
    u8g2.drawStr(0, 34, buffer);

    snprintf(buffer, sizeof(buffer), "Relay: %s", relayState ? "ON" : "OFF");
    u8g2.drawStr(0, 46, buffer);

    if (isCountdownActive) {
        snprintf(buffer, sizeof(buffer), "Tempo: %d s", countdown);
        u8g2.drawStr(0, 58, buffer);
    }

    // Afficher le mode en haut à droite
    u8g2.setFont(u8g2_font_5x8_tr);
    snprintf(buffer, sizeof(buffer), "%s", isAutoMode ? "AUTO" : "MANUAL");
    u8g2.drawStr(90, 10, buffer);

    // Point clignotant
    if (isDotVisible_) {
        u8g2.drawStr(120, 58, ".");
    }
    isDotVisible_ = !isDotVisible_;

    u8g2.sendBuffer();
}
