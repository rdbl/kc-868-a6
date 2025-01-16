# kc868-A6 – Insert Bouilleur Autonome

## Description du projet

Ce projet a pour but de créer une régulation autonome pour un **insert bouilleur** (un foyer de cheminée qui chauffe de l’eau) en utilisant la carte **kc868-A6**. Les objectifs principaux sont :

- **Régulation autonome** pour fonctionner même si le Wi-Fi est hors service.  
- **Envoi des informations** de fonctionnement vers Home Assistant (et pilotage depuis Home Assistant).  
- **LoRa** pour transmettre les données vers une passerelle centralisant l’ensemble du système de chauffage.  
- **Interface web** accessible via un point d’accès (AP) local, permettant de configurer l’appareil ou d’intervenir en cas de panne Wi-Fi.  
- **Capteurs DS18B20** pour mesurer la température, notamment sur deux sondes distinctes (par exemple, eau d’entrée et eau de sortie).  

## Composants utilisés

1. **Carte kc868-A6** : Le cerveau du projet, qui exécute la logique de régulation et gère les communications.  
2. **Capteurs DS18B20** : Mesurent précisément la température. Deux sondes sont utilisées pour comparer la température d’entrée et de sortie.  
3. **Module Wi-Fi** : Permet le dialogue avec Home Assistant et la configuration à distance.  
4. **Module LoRa** : Assure une transmission longue portée vers une gateway centralisée.  
5. **Actionneurs** (pompes, vannes, etc.) : Gérés par la carte pour activer ou couper le flux d’eau en fonction des températures lues.  

## Fonctionnalité de différentiel de température

Le système compare les valeurs de **sonde 1** et **sonde 2** (deux DS18B20).  
- **Différentiel** : possibilité de régler à distance l’écart de température (ex. : 5 °C, 10 °C, etc.) qui déclenche la pompe ou la désactive.  
- **Exemple** : si la sonde 1 lit 60 °C et la sonde 2 lit 50 °C, et que le différentiel est de 10 °C, alors la pompe reste éteinte. Si la sonde 1 monte à 65 °C (et la sonde 2 reste à 50 °C), la différence devient 15 °C, ce qui déclenche la pompe pour faire circuler l’eau.  

Cette consigne peut être ajustée :
- **Depuis Home Assistant**, via l’intégration dédiée.  
- **Via l’interface web** disponible sur le point d’accès local.

## Roadmap

1. **Mise en place de la régulation autonome**  
   - Lecture des capteurs DS18B20 (sonde 1 et sonde 2).  
   - Calcul du différentiel (en °C) pour l’activation/désactivation de la pompe.  
   - Gestion des alertes (surchauffe, température trop basse, etc.).  

2. **Intégration Home Assistant**  
   - Publication des données : température, état de la pompe, etc.  
   - Modification à distance du différentiel.  

3. **Implémentation LoRa**  
   - Configuration du module LoRa pour communiquer avec la passerelle existante.  
   - Remontée des informations de température et d’état de la pompe vers un serveur central.  

4. **Interface web de configuration**  
   - Accès via le point d’accès local (AP) pour :  
     - Configurer le Wi-Fi.  
     - Régler le différentiel de température.  
     - Visualiser l’état des capteurs.  
   - Accès possible même en l’absence d’internet ou de routeur externe.
