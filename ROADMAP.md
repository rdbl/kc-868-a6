### **📌 Récapitulatif Synthétique du Fonctionnement**  

L'objectif est de créer **une architecture modulaire et découplée** où **toutes les interactions entre modules passent par `EventManager`**.  

#### **🛠️ Composants principaux :**
1. **`DataStore` (Lecture seule)** → Stocke les données du système, mais **ne peut être modifié que par `EventManager`**.
2. **`EventManager` (Ancien `MessageBus`)** →  
   - 📢 **Publie** des événements (`publish(event, value)`).  
   - 👂 **Écoute** les événements (`subscribe(event, callback)`).  
   - ⏳ **Planifie** des événements différés (`schedule(event, value, delayMs, priority)`).  
   - ⚙️ **Met à jour `DataStore` automatiquement** lorsqu'un événement est publié.
3. **`MQTTManager` / `LoRaManager` / `WiFiManager`** →  
   - Publient et écoutent les événements via `EventManager`, sans dépendance directe entre eux.
4. **`HardwareManager`** →  
   - **Capteurs et actionneurs interagissent via `EventManager`**, sans dépendre de `MQTT` ou `LoRa`.

---

### **📌 Roadmap pour l’implémentation**
🔥 **On va avancer progressivement, en testant chaque étape !**  

#### **🔹 Étape 1 : Mettre en place `EventManager` avec `DataStore`**
✅ Créer `EventManager` avec `publish()`, `subscribe()` et `schedule()`.  
✅ Intégrer la mise à jour automatique du `DataStore` dans `publish()`.  
✅ Tester `EventManager` en isolation pour vérifier qu'il fonctionne. 

#### **🔹 Étape 2 : Intégrer `HardwareManager` avec `EventManager`**
✅ Lire des capteurs et **publier leurs valeurs** via `EventManager`.  
✅ Écouter les événements pour **activer/désactiver les actionneurs**. 

#### **🔹 Étape 3 : Gestion avancée des événements**
✅ Ajout des priorités et délais dans `EventManager`.  
✅ Ajout de `processScheduledEvents()` pour exécuter les événements planifiés.  
✅ Vérifier que les événements sont **bien différés et exécutés au bon moment**.  

#### **🔹 Étape 4 : Ajouter `MQTTManager`, `LoRaManager` et `WiFiManager`**
✅ `MQTTManager` écoute `EventManager` et publie les données.  
✅ `LoRaManager` et `WiFiManager` récupèrent les événements et les transmettent.  
✅ Vérifier que **les communications sont bien synchronisées** avec `EventManager`.  



---

### **📌 Objectif final**
🚀 **Avoir un système où tous les modules sont totalement indépendants, communiquent via `EventManager`, et où `DataStore` est mis à jour proprement sans accès direct.**  

