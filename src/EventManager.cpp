#include "EventManager.h"

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

EventManager::EventManager() : store(DataStore::getInstance()) {}

// Surcharge pour événement sans valeur
void EventManager::subscribe(const String& event, std::function<void()> callback) {
    subscribers[event].noValueCallbacks.push_back(callback);
    Serial.printf("[EVENTMANAGER] Abonnement à l'événement '%s' sans valeur\n", event.c_str());
}

// Surcharge pour événement sans valeur
void EventManager::publish(const String& event) {
    Serial.printf("[EVENTMANAGER] Événement publié (sans valeur) : %s\n", event.c_str());
    
    // Vérifier s'il y a des abonnés pour cet événement
    if (subscribers.find(event) != subscribers.end()) {
        // Exécuter les callbacks sans valeur
        for (auto& callback : subscribers[event].noValueCallbacks) {
            callback();
        }
    }

}

// Surcharge pour planification sans valeur
void EventManager::schedule(const String& event, unsigned long delayMs, int priority) {
    unsigned long executionTime = millis() + delayMs;
    
    // Créer un événement planifié
    ScheduledEvent scheduledEvent;
    scheduledEvent.executionTime = executionTime;
    scheduledEvent.eventName = event;
    scheduledEvent.priority = priority;
    scheduledEvent.hasValue = false;
    scheduledEvent.valueType = DataType::UNKNOWN;
    
    // Ajouter l'événement à la file d'attente
    scheduledEvents.push(scheduledEvent);
    
    Serial.printf("[EVENTMANAGER] Événement planifié (sans valeur) : %s dans %lu ms (Priorité: %d)\n", 
                 event.c_str(), delayMs, priority);
}

void EventManager::processScheduledEvents() {
    unsigned long currentTime = millis();

    while (!scheduledEvents.empty() && scheduledEvents.top().executionTime <= currentTime) {
        ScheduledEvent event = scheduledEvents.top();
        scheduledEvents.pop();

        if (event.hasValue) {
            // Récupérer la valeur du JsonDocument
            JsonVariant value = event.valueDoc.as<JsonVariant>();
            
            Serial.printf("[EVENTMANAGER] Exécution d'un événement planifié avec valeur : %s\n", event.eventName.c_str());
            
            // Traiter selon le type stocké
            switch (event.valueType) {
                case DataType::BOOL:
                    publish(event.eventName, value.as<bool>());
                    break;
                case DataType::INT:
                    publish(event.eventName, value.as<int>());
                    break;
                case DataType::UINT:
                    publish(event.eventName, value.as<unsigned int>());
                    break;
                case DataType::LONG:
                    publish(event.eventName, value.as<long>());
                    break;
                case DataType::ULONG:
                    publish(event.eventName, value.as<unsigned long>());
                    break;
                case DataType::FLOAT:
                    publish(event.eventName, value.as<float>());
                    break;
                case DataType::DOUBLE:
                    publish(event.eventName, value.as<double>());
                    break;
                case DataType::STRING:
                    publish(event.eventName, value.as<String>());
                    break;
                case DataType::CHAR_PTR:
                    publish(event.eventName, value.as<const char*>());
                    break;
                default:
                    // Type non reconnu, on publie sans valeur
                    Serial.printf("[EVENTMANAGER] Type non reconnu pour l'événement planifié : %s\n", event.eventName.c_str());
                    publish(event.eventName);
                    break;
            }
        } else {
            Serial.printf("[EVENTMANAGER] Exécution d'un événement planifié sans valeur : %s\n", event.eventName.c_str());
            publish(event.eventName);
        }
    }
}

void EventManager::printScheduledEvents() {
    if (scheduledEvents.empty()) {
        Serial.println("[SCHEDULE] Aucun événement planifié.");
        return;
    }

    std::priority_queue<ScheduledEvent> tempQueue = scheduledEvents;
    unsigned long now = millis();

    Serial.println("[SCHEDULE] Événements planifiés :");
    while (!tempQueue.empty()) {
        ScheduledEvent e = tempQueue.top();
        tempQueue.pop();

        long timeLeft = (long)(e.executionTime - now);
        if (timeLeft < 0) timeLeft = 0;
        long secondsLeft = timeLeft / 1000; // conversion en secondes

        if (e.hasValue) {
            Serial.printf(" - Event: %s (avec valeur de type %s) dans %ld sec (priorité %d)\n",
                        e.eventName.c_str(),
                        dataTypeToString(e.valueType).c_str(),
                        secondsLeft,
                        e.priority);
        } else {
            Serial.printf(" - Event: %s (sans valeur) dans %ld sec (priorité %d)\n",
                        e.eventName.c_str(),
                        secondsLeft,
                        e.priority);
        }
    }
}