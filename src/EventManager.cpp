#include "EventManager.h"

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

EventManager::EventManager() : store(DataStore::getInstance()) {}

// ✅ `subscribe()` : S'abonner à un événement
void EventManager::subscribe(const String& event, std::function<void(float)> callback) {
    subscribers[event].push_back(callback);
}

// ✅ `publish()` : Publier un événement immédiatement + Mettre à jour `DataStore`
void EventManager::publish(const String& event, float value) {
    Serial.printf("[EVENTMANAGER] Événement publié : %s - Valeur : %.2f\n", event.c_str(), value);
    
    store.set(event, value);
    store.printStore();  // ✅ Afficher l'état du `DataStore` après chaque mise à jour

    if (subscribers.find(event) != subscribers.end()) {
        for (auto& callback : subscribers[event]) {
            callback(value);
        }
    }
}


// ✅ `schedule()` : Planifier un événement différé
void EventManager::schedule(const String& event, float value, unsigned long delayMs, int priority) {
    unsigned long executionTime = millis() + delayMs;
    scheduledEvents.push({executionTime, event, value, priority});
    Serial.printf("[EVENTMANAGER] Événement planifié : %s dans %lu ms (Priorité: %d)\n", event.c_str(), delayMs, priority);
}

// ✅ `processScheduledEvents()` : Exécuter les événements planifiés
void EventManager::processScheduledEvents() {
    unsigned long currentTime = millis();

    while (!scheduledEvents.empty() && scheduledEvents.top().executionTime < currentTime) {
        ScheduledEvent event = scheduledEvents.top();
        scheduledEvents.pop();

        // 🔹 Mettre à jour `DataStore` lors de l'exécution d'un événement planifié
        store.set(event.eventName, event.value);

        // 🔹 Publier l'événement pour informer les abonnés
        publish(event.eventName, event.value);
    }
}

void EventManager::printScheduledEvents() {
    if (scheduledEvents.empty()) {
        Serial.println("[SCHEDULE] Aucun événement planifié.");
        return;
    }

    // 🔹 Faire une copie de la priority_queue
    std::priority_queue<ScheduledEvent> tempQueue = scheduledEvents;

    unsigned long now = millis();

    Serial.println("[SCHEDULE] Événements planifiés :");
    while (!tempQueue.empty()) {
        ScheduledEvent e = tempQueue.top();
        tempQueue.pop();

        long timeLeft = (long)(e.executionTime - now);
        if (timeLeft < 0) timeLeft = 0;
        long secondsLeft = timeLeft / 1000; // conversion en secondes

        Serial.printf(" - Event: %s dans %ld sec (priorité %d)\n",
                      e.eventName.c_str(),
                      secondsLeft,
                      e.priority);
    }
}
