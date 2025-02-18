#include <ArduinoOTA.h>

/************************************
 *       FONCTION : setupOTA
 ************************************/
void setupOTA()
{
  // Nom d’hôte du module (visible dans l’IDE Arduino lors de l’OTA)
  ArduinoOTA.setHostname("regulation-bouilleur");

  // Callbacks pour le debug
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { 
        type = "filesystem";
      }
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress * 100) / total); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)  Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)    Serial.println("End Failed"); });

  // Initialisation de l’OTA
  ArduinoOTA.begin();
  Serial.println("OTA Ready - Connecté au WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
