#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>
#include "EventManager.h"
#include "DataStore.h"

/**
 * @brief Classe de base pour tous les modules du système
 * 
 * Cette classe définit l'interface commune que tous les modules
 * doivent implémenter pour s'intégrer correctement au système.
 */
class Module {
public:
    /**
     * @brief Initialiser le module
     * 
     * Cette méthode doit être appelée une fois, généralement dans setup(),
     * pour initialiser le module et ses dépendances.
     */
    virtual void initialize() = 0;
    
    /**
     * @brief Vérifier si le module est initialisé
     * @return true si le module est initialisé, false sinon
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Obtient le nom du module
     * @return String - Le nom du module
     */
    virtual String getName() const = 0;
    
    /**
     * @brief Méthode appelée régulièrement dans la boucle principale
     * 
     * Cette méthode permet au module d'effectuer des tâches périodiques
     * qui ne sont pas gérées par des événements planifiés.
     */
    virtual void loop() {}
    
protected:
    // Références aux singletons partagés
    EventManager& eventManager_ = EventManager::getInstance();
    DataStore& dataStore_ = DataStore::getInstance();
};

#endif // MODULE_H