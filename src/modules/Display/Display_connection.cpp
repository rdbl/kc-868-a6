#include "display_connection.h"
#include <Arduino.h>




// Constructeur : n'appelle pas u8g2.begin() immédiatement
DisplayConnection::DisplayConnection() : u8g2(U8G2_R0, 15, 4, U8X8_PIN_NONE) {
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
        return;
    }

    // Serial.println("[INFO] Updating display...");
    // Serial.println("[INFO] Temp1: "); Serial.println(temp1);
    // Serial.println("[INFO] Temp2: "); Serial.println(temp2);
    // Serial.println("[INFO] Temp diff: "); Serial.println(tempDifference);
    // Serial.println("[INFO] Relay state: "); Serial.println(relayState);
    // Serial.println("[INFO] Countdown: "); Serial.println(countdown);
    // Serial.println("[INFO] Countdown active: "); Serial.println(isCountdownActive);
    // Serial.println("[INFO] Auto mode: "); Serial.println(isAutoMode);
    

    // Effacer le tampon
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "S1(bleu): %.2f°C", temp1);
    u8g2.drawStr(0, 10, buffer);

    snprintf(buffer, sizeof(buffer), "S2(blanc): %.2f°C", temp2);
    u8g2.drawStr(0, 22, buffer);

    snprintf(buffer, sizeof(buffer), "Diff: %.2f°C", tempDifference);
    u8g2.drawStr(0, 34, buffer);

    snprintf(buffer, sizeof(buffer), "Pump: %s", relayState ? "ON" : "OFF");
    u8g2.drawStr(0, 46, buffer);

    if (isCountdownActive) {
        snprintf(buffer, sizeof(buffer), "Tempo: %d s", countdown);
        u8g2.drawStr(0, 58, buffer);
    }else{
        u8g2.drawStr(0, 58, "Tempo: -");
    }

    // Afficher le mode en bas à droite, juste au-dessus du point clignotant
    // u8g2.setFont(u8g2_font_5x8_tr);
    snprintf(buffer, sizeof(buffer), "%s", isAutoMode ? "AUTO" : "MANUAL");
    int text_width = u8g2.getStrWidth(buffer);
    int x = u8g2.getDisplayWidth() - text_width - 10;  // Position x : aligné à droite
    int y = 45; // Position y choisie pour être juste au-dessus du point clignotant (affiché à y=58)
    u8g2.drawStr(x, y, buffer);

    // Point clignotant
    if (isDotVisible_) {
        u8g2.drawStr(120, 58, ".");
    }
    isDotVisible_ = !isDotVisible_;

    u8g2.sendBuffer();
}

