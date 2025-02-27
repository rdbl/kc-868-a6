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
    JsonDocument valueDoc;  // Document pour stocker la valeur
    int priority;
    bool hasValue;      // Indique si l'événement a une valeur ou non
    DataType valueType; // Le type de la valeur stockée
    
    bool operator<(const ScheduledEvent& other) const {
        return priority < other.priority;  // Plus la priorité est haute, plus l'élément est exécuté rapidement
    }
};

class EventManager {
public:
    static EventManager& getInstance();

    // Surcharge pour événement sans valeur
    void subscribe(const String& event, std::function<void()> callback);
    
    // Surcharge pour événement avec valeur typée
    template <typename T>
    void subscribe(const String& event, std::function<void(T)> callback);

    // Rétrocompatibilité avec l'ancienne version
    void subscribe(const String& event, std::function<void(float)> callback) {
        subscribe<float>(event, callback);
    }

    // Surcharge pour événement sans valeur
    void publish(const String& event);
    
    // Surcharge pour événement avec valeur typée
    template <typename T>
    void publish(const String& event, T value);

    // Rétrocompatibilité avec l'ancienne version
    void publish(const String& event, float value) {
        publish<float>(event, value);
    }
    
    // Surcharge pour planification sans valeur
    void schedule(const String& event, unsigned long delayMs, int priority = 0);
    
    // Surcharge pour planification avec valeur typée
    template <typename T>
    void schedule(const String& event, T value, unsigned long delayMs, int priority = 0);

    // Rétrocompatibilité avec l'ancienne version
    void schedule(const String& event, float value, unsigned long delayMs, int priority = 0) {
        schedule<float>(event, value, delayMs, priority);
    }
    
    void processScheduledEvents();  // Doit être appelé dans loop()
    void printScheduledEvents();

private:
    EventManager();
    
    // Structure plus complexe pour stocker différents types de callbacks
    struct EventCallbacks {
        std::vector<std::function<void()>> noValueCallbacks;
        std::map<DataType, std::vector<std::function<void(JsonVariant)>>> typedCallbacks;
    };
    
    std::map<String, EventCallbacks> subscribers;
    std::priority_queue<ScheduledEvent> scheduledEvents;
    DataStore& store;
};

// Implémentation des templates
template <typename T>
void EventManager::subscribe(const String& event, std::function<void(T)> callback) {
    // Obtenir le type de données pour T
    DataType type = getDataType<T>();
    
    // Créer un wrapper qui convertit JsonVariant en T
    auto wrapper = [callback](JsonVariant value) {
        callback(value.as<T>());
    };
    
    // Ajouter ce wrapper aux callbacks du bon type
    subscribers[event].typedCallbacks[type].push_back(wrapper);
    
    Serial.printf("[EVENTMANAGER] Abonnement à l'événement '%s' avec type %s\n", 
                event.c_str(), dataTypeToString(type).c_str());
}

template <typename T>
void EventManager::publish(const String& event, T value) {
    Serial.printf("[EVENTMANAGER] Événement publié : %s\n", event.c_str());
    
    // Mettre à jour le DataStore
    store.set(event, value);
    
    // Afficher le contenu du DataStore
    store.printStore();
    
    // Vérifier s'il y a des abonnés pour cet événement
    if (subscribers.find(event) != subscribers.end()) {
        // Obtenir le type de T
        DataType type = getDataType<T>();
        
        // Exécuter les callbacks sans valeur
        for (auto& callback : subscribers[event].noValueCallbacks) {
            callback();
        }
        
        // Exécuter les callbacks du bon type
        auto& typedCallbacks = subscribers[event].typedCallbacks;
        if (typedCallbacks.find(type) != typedCallbacks.end()) {
            // Créer un JsonDocument temporaire pour stocker la valeur
            JsonDocument doc;
            JsonVariant jsonValue = doc.to<JsonVariant>();
            // Affecter la valeur au JsonVariant (utilisation de la méthode set)
            jsonValue.set(value);
            
            for (auto& callback : typedCallbacks[type]) {
                callback(jsonValue);
            }
        }
    }
}

template <typename T>
void EventManager::schedule(const String& event, T value, unsigned long delayMs, int priority) {
    unsigned long executionTime = millis() + delayMs;
    
    // Créer un événement planifié
    ScheduledEvent scheduledEvent;
    scheduledEvent.executionTime = executionTime;
    scheduledEvent.eventName = event;
    scheduledEvent.priority = priority;
    scheduledEvent.hasValue = true;
    scheduledEvent.valueType = getDataType<T>();
    
    // Stocker la valeur dans le JsonDocument de l'événement
    JsonVariant jsonValue = scheduledEvent.valueDoc.to<JsonVariant>();
    jsonValue.set(value);
    
    // Ajouter l'événement à la file d'attente
    scheduledEvents.push(scheduledEvent);
    
    Serial.printf("[EVENTMANAGER] Événement planifié avec valeur : %s dans %lu ms (Priorité: %d)\n", 
                 event.c_str(), delayMs, priority);
}

#endif