#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <vector>
#include "Module.h"

/**
 * @brief Gestionnaire central des modules du système
 * 
 * Cette classe gère le cycle de vie de tous les modules
 * et simplifie leur intégration dans le système.
 */
class ModuleManager {
public:
    // Singleton
    static ModuleManager& getInstance();
    
    /**
     * @brief Enregistrer un module dans le gestionnaire
     * @param module Référence vers le module à enregistrer
     */
    void registerModule(Module& module);
    
    /**
     * @brief Initialiser tous les modules enregistrés
     */
    void initializeAllModules();
    
    /**
     * @brief Exécuter la méthode loop() de tous les modules
     * Doit être appelée dans la boucle principale
     */
    void loopAllModules();
    
    /**
     * @brief Afficher l'état de tous les modules
     */
    void printModulesStatus();

private:
    ModuleManager() {}
    std::vector<Module*> modules_;
};

#endif // MODULE_MANAGER_H