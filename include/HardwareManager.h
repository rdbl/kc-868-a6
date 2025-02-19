#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <ArduinoJson.h>
#include "PCF8574.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Display_connection.h"

// On pourra également inclure d'autres headers selon les modules à gérer

class HardwareManager {
  public:
    HardwareManager();
    ~HardwareManager();

    // Initialise l'ensemble du matériel à partir de la configuration globale
    void begin();

    void scanI2CBus(TwoWire* bus);
    // Accesseurs pour les objets créés (pour pouvoir les utiliser dans la logique applicative)
    DallasTemperature* getSensor(int index);  // Pour récupérer le capteur DS18B20 correspondant
    PCF8574* getPCF8574();

    // Vous pouvez ajouter des getters pour d'autres composants : displays, actuators, etc.
    
  private:
    // Pour gérer les capteurs DS18B20 (nous allons stocker ces objets dans un tableau dynamique)
    OneWire** sensorOneWires;
    DallasTemperature** sensorTemps;
    int sensorCount;

    // Pour le module PCF8574 (gestion des relais)
    TwoWire* i2cBus;
    PCF8574* pcf8574_R1;
    
    // Vous pouvez aussi stocker ici d'autres objets matériels, par exemple une instance de DisplayConnection, etc.
    
    // Fonctions internes pour extraire la configuration et instancier les objets
    void initSensors();
    void initPCF8574();
};

#endif
