#include "HardwareManager.h"

#define SDA_PIN 4   // ✅ Définir correctement la broche SDA
#define SCL_PIN 15  // ✅ Définir correctement la broche SCL

HardwareManager& HardwareManager::getInstance() {
    static HardwareManager instance;
    return instance;
}

HardwareManager::HardwareManager() : i2cBus(0) {  // ✅ Initialise TwoWire sur le bus 0
    sensorOneWires[0] = nullptr;
    sensorOneWires[1] = nullptr;
    sensorTemps[0] = nullptr;
    sensorTemps[1] = nullptr;
    pcf8574 = nullptr;
}

void HardwareManager::initialize() {
    Serial.println("[HARDWARE] Initialisation en cours...");
    // 🔹 Initialisation I2C avec les bonnes broches
    i2cBus.begin(SDA_PIN, SCL_PIN, 100000);

    // 🔹 Initialisation du PCF8574 avec `TwoWire`
    pcf8574 = new PCF8574(&i2cBus, 0x24);
    bool connected = pcf8574->begin();

    for(int i = 0; i < 6; i++){
      pcf8574->pinMode(i, OUTPUT);
      pcf8574->digitalWrite(i, HIGH);
    }

    // 🔹 Vérification de la connexion au PCF8574
    if (pcf8574->begin()) {
        Serial.println("[HARDWARE] PCF8574 détecté !");
    } else {
        Serial.println("[HARDWARE] ⚠️ ERREUR : PCF8574 non détecté !");
        Serial.println("🔍 Scan I2C en cours...");
        for (byte addr = 1; addr < 127; addr++) {
          i2cBus.beginTransmission(addr);
            if (i2cBus.endTransmission() == 0) {
                Serial.printf("✅ Périphérique trouvé à l'adresse 0x%02X\n", addr);
            }else{
                Serial.printf("❌ Aucun périphérique trouvé à l'adresse 0x%02X\n", addr);
            }
        }
    }

    // 🔹 Initialisation des capteurs DS18B20
    sensorOneWires[0] = new OneWire(32);
    sensorOneWires[1] = new OneWire(33);
    sensorTemps[0] = new DallasTemperature(sensorOneWires[0]);
    sensorTemps[1] = new DallasTemperature(sensorOneWires[1]);

    sensorTemps[0]->begin();
    sensorTemps[1]->begin();

    // ✅ Abonnement à "RelayControl"
    EventManager::getInstance().subscribe<bool>("RelayControl", [this](bool state) {
      Serial.printf("[HARDWARE] Changement état relais : %s\n", state ? "ON" : "OFF");
      pcf8574->digitalWrite(0, state ? HIGH : LOW);
    });

    // ✅ Abonnement à "UpdateSensors"
    EventManager::getInstance().subscribe("UpdateSensors", [this]() {
      Serial.println("[HARDWARE] Mise à jour des capteurs...");
      updateSensors();
    });


    Serial.println("[HARDWARE] Initialisation terminée !");
   
}

  // ✅ Mise à jour des capteurs et publication via `EventManager`
void HardwareManager::updateSensors() {
  sensorTemps[0]->requestTemperatures();
  sensorTemps[1]->requestTemperatures();

  float temp1 = sensorTemps[0]->getTempCByIndex(0);
  float temp2 = sensorTemps[1]->getTempCByIndex(0);

  // 🔥 Publier les températures via `EventManager`
  EventManager::getInstance().publish("Sensor.0.Temperature", temp1);
  EventManager::getInstance().publish("Sensor.1.Temperature", temp2);
}

void HardwareManager::handleActuators() {
    bool relayState = pcf8574->digitalRead(0);
    Serial.printf("[HARDWARE] État actuel du relais (lecture) : %s\n", relayState ? "ON" : "OFF");
}