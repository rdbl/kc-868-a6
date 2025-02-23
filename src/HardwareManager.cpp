#include "HardwareManager.h"
#include "LoadConfig.h"  // Pour accéder à l'objet global 'config'
#include <stdlib.h>      // Pour strtol

HardwareManager::HardwareManager() 
  : sensorOneWires(nullptr), sensorTemps(nullptr), sensorCount(0), i2cBus(nullptr), pcf8574_R1(nullptr) 
{
}

HardwareManager::~HardwareManager() {
  // Libérer les capteurs
  for (int i = 0; i < sensorCount; i++) {
    delete sensorTemps[i];
    delete sensorOneWires[i];
  }
  delete[] sensorTemps;
  delete[] sensorOneWires;
  
  // Libérer le PCF8574 (si nécessaire)
  delete pcf8574_R1;
  // Si i2cBus a été alloué dynamiquement, le libérer ici aussi (souvent, on peut l'instancier statiquement)
  // delete i2cBus;
}

void HardwareManager::scanI2CBus(TwoWire* bus) {
  Serial.println("Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    bus->beginTransmission(addr);
    if (bus->endTransmission() == 0) {
      Serial.print("Device found at address 0x");
      Serial.println(addr, HEX);
    }
  }
}

void HardwareManager::begin() {
  // On part du principe que loadConfig() a déjà été exécuté.
  //debug message   
  Serial.println("HardwareManager::begin()");
  initSensors();
  Serial.println("HardwareManager::initSensors()");
  initPCF8574();
  Serial.println("HardwareManager::initPCF8574()");
  scanI2CBus(i2cBus);
  // Vous pourrez ici ajouter d'autres initialisations (affichages, actuators, etc.)
}

//////////////////////
// Initialisation des capteurs
//////////////////////
void HardwareManager::initSensors() {
  // On suppose que config.sensors contient la configuration de tous les capteurs DS18B20
  Serial.println("HardwareManager::initSensors()");
  sensorCount = config.sensorsCount;
  Serial.print("sensorCount: ");
  Serial.println(sensorCount);
  if (sensorCount <= 0) return;

  sensorOneWires = new OneWire*[sensorCount];
  sensorTemps = new DallasTemperature*[sensorCount];

  //debug message
    Serial.println("sensorOneWires et sensorTemps alloués");

  // Pour chaque capteur, extraire la pin depuis la configuration
  for (int i = 0; i < sensorCount; i++) {
    int pin = 0;
    {
      // Utilisation d'un JsonDocument temporaire pour extraire la valeur de pin
      JsonDocument hwDoc;
      // On suppose que config.sensors[i].hardwareJson contient un objet JSON (par exemple : {"pins": {"COM": 33}})
      deserializeJson(hwDoc, config.sensors[i].hardwareJson);
      pin = hwDoc["pins"]["COM"] | 0;
    }
    //debug messages 
    Serial.print("pin: "); 
    Serial.println(pin);

    sensorOneWires[i] = new OneWire(pin);
    sensorTemps[i] = new DallasTemperature(sensorOneWires[i]);
    sensorTemps[i]->begin();
    //debug message
    Serial.println("sensorTemps[i]->begin()");
  }
}

DallasTemperature* HardwareManager::getSensor(int index) {
  if (index >= 0 && index < sensorCount){
    return sensorTemps[index];
    }
  return nullptr;
}

//////////////////////
// Initialisation du module PCF8574
//////////////////////
JsonDocument HardwareManager::initPCF8574() {
  // Extraction de la configuration pour le module PCF8574 depuis config.device.hardwareJson
  int pcfAddress = 0, pcfR1First = 0, pcfR1Last = 0, i2csda= 0, i2cscl=0, num_relays=0;
  
  // Utilisation d'un JsonDocument temporaire pour extraire la valeur de pcfAddress
  JsonDocument hwDoc;
  deserializeJson(hwDoc, config.device.hardwareJson);
  // On suppose que la configuration est structurée comme :
  // {"i2c": {"pcf8574": {"address": "0x24", "num_relays": 6, "R1": {"pins": {"first": 4, "last": 15}}}}}
  const char* addrStr = hwDoc["i2c"]["pcf8574"]["address_hex"];
  pcfAddress = (int)strtol(addrStr, NULL, 0);
  pcfR1First = hwDoc["i2c"]["pcf8574"]["R1"]["pins"]["first"] | 0;
  pcfR1Last  = hwDoc["i2c"]["pcf8574"]["R1"]["pins"]["last"]  | 0;
  i2csda = hwDoc["i2c"]["sda"] | 0;
  i2cscl = hwDoc["i2c"]["scl"] | 0;
  num_relays = hwDoc["i2c"]["pcf8574"]["num_relays"] | 0;
  Serial.print("num_relays: ");
  Serial.println(num_relays);
  

  // Instanciation de l'interface I2C (vous pouvez décider de le créer ici ou le déclarer globalement)
  i2cBus = new TwoWire(0);
  // i2cBus->begin(i2csda, i2cscl);
  
  pcf8574_R1 = new PCF8574(i2cBus, pcfAddress, pcfR1First, pcfR1Last);  
  
  // Ajoutez ici l'initialisation complète du module PCF8574
  pcf8574_R1->begin();
  for(int i = 0; i < num_relays; i++){
    pcf8574_R1->pinMode(i, OUTPUT);
    pcf8574_R1->digitalWrite(i, HIGH);
    hwDoc["actuators"][i]["params"]["relayState"] = "HIGH";
    hwDoc["actuators"][i]["params"]["IsEnabled"] = false;
  }
  
  hwDoc["actuators"][0]["params"]["IsEnabled"] = true;
  
  return hwDoc;

}


PCF8574* HardwareManager::getPCF8574() {
  return pcf8574_R1;
}
