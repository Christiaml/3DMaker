#include <ESP8266WiFi.h>
#include <strings_en.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>

//Inicio del temporizador ticker y funcion de verificacion de conexion por parpadeo de led
#define ledWifi D4
Ticker tic_WifiLed;
void ledParpadeoWifi(){
  byte estado = digitalRead(ledWifi);
  digitalWrite(ledWifi, !estado);
}

void connectWifi_Man(){
  //Comunicacion Serial
  Serial.begin(115200);

  //Led indicando conexion
  pinMode(ledWifi,OUTPUT);  
  tic_WifiLed.attach(0.3, ledParpadeoWifi);

  //Instancia del Manager
  WiFiManager wifiManager;

  //Descomentar para limpiar credenciales de WiFi existentes
  //wifiManager.resetSettings();
  
  //Creacion de AP
  if(!wifiManager.autoConnect("ESP8266_Maker", "12345678")){
    Serial.println("Fallo en la conexion (timeout)");
    ESP.reset();
    delay(1000);
  } 
  
  Serial.println("Ya estas Conectado");
  tic_WifiLed.detach();

  digitalWrite(ledWifi, HIGH);
}
