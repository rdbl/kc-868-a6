#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <Wire.h>
#include <PCF8574.h>
#include <DallasTemperature.h>
#include "EventManager.h"

class HardwareManager {
public:
    static HardwareManager& getInstance();
    void initialize();
    void updateSensors();
    void handleActuators();

private:
    HardwareManager();
    TwoWire i2cBus;  // ✅ Instance personnalisée de TwoWire
    PCF8574* pcf8574;
    OneWire* sensorOneWires[2];
    DallasTemperature* sensorTemps[2];
};

#endif
