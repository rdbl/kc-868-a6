#include "ModuleManager.h"

// Implémentation du singleton
ModuleManager& ModuleManager::getInstance() {
    static ModuleManager instance;
    return instance;
}

// Enregistrer un module
void ModuleManager::registerModule(Module& module) {
    modules_.push_back(&module);
    Serial.printf("[MODULE] Module '%s' enregistré\n", module.getName().c_str());
}

// Initialiser tous les modules
void ModuleManager::initializeAllModules() {
    Serial.println("[MODULE] Initialisation de tous les modules...");
    
    for (auto module : modules_) {
        Serial.printf("[MODULE] Initialisation du module '%s'...\n", module->getName().c_str());
        module->initialize();
        
        if (module->isInitialized()) {
            Serial.printf("[MODULE] Module '%s' initialisé avec succès\n", module->getName().c_str());
        } else {
            Serial.printf("[MODULE] ERREUR: Échec de l'initialisation du module '%s'\n", module->getName().c_str());
        }
    }
    
    Serial.println("[MODULE] Tous les modules ont été initialisés");
}

// Exécuter la méthode loop() de tous les modules
void ModuleManager::loopAllModules() {
    for (auto module : modules_) {
        if (module->isInitialized()) {
            module->loop();
        }
    }
}

// Afficher l'état de tous les modules
void ModuleManager::printModulesStatus() {
    Serial.println("\n--- ÉTAT DES MODULES ---");
    
    for (auto module : modules_) {
        Serial.printf("Module '%s': %s\n", 
                     module->getName().c_str(), 
                     module->isInitialized() ? "Initialisé" : "Non initialisé");
    }
    
    Serial.println("---------------------\n");
}