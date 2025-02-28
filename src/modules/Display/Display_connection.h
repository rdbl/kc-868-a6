#ifndef DISPLAY_CONNECTION_H
#define DISPLAY_CONNECTION_H

#include <U8G2lib.h>

class DisplayConnection {
public:
    DisplayConnection(); // Constructeur
    void begin();  // <-- Nouvelle fonction pour initialiser le display
    void displayInfos(float temp1, float temp2, float tempDifference, bool relayState, 
                      int countdown, bool isCountdownActive, bool isAutoMode);

private:
    bool isDotVisible_ = false;
    bool oled_initialized_ = false; // Indique si l'OLED a bien été détecté
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;
};

#endif
