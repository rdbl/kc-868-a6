#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <map>
#include <vector>
#include <queue>
#include <functional>
#include <Arduino.h>
#include "DataStore.h"

struct ScheduledEvent {
    unsigned long executionTime;
    String eventName;
    float value;
    int priority;
    
    bool operator<(const ScheduledEvent& other) const {
        return priority < other.priority;  // Plus la priorité est haute, plus l'élément est exécuté rapidement
    }
};

class EventManager {
public:
    static EventManager& getInstance();

    void subscribe(const String& event, std::function<void(float)> callback);
    void publish(const String& event, float value);
    
    void schedule(const String& event, float value, unsigned long delayMs, int priority = 0);
    void processScheduledEvents();  // Doit être appelé dans loop()
    void printScheduledEvents();


private:
    EventManager();
    std::map<String, std::vector<std::function<void(float)>>> subscribers;
    std::priority_queue<ScheduledEvent> scheduledEvents;
    DataStore& store;
};

#endif
