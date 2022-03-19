#include "Wifi_Man.h"
#include <PID_v1.h>
#include <Ubidots.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

//Nombre dispositivo UBIDOTS
#define DEVICE_LABEL "ESP8266-Control"

#define VCPP D5
#define VF1 D6
#define VF2 D7
#define SSR D8

//Pin de conexion del Relay de 30A
#define R_30A D2

//Valores para control de temperatura y funcionamiento de ventiladores
double Setpoint, Input, Output;
float Kp = 2;
float Kd = 5;
float Ki = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
int WindowSize = 2000;
int WindowSizeVPP = 6000;
unsigned long windowStartTime;
int S_SSR = 0;

//Setup Bus DS18B20
#define oneWireBus D1
OneWire ow(oneWireBus);

//Dallas Temperature setup
DallasTemperature Sensors(&ow);
int NumbOfDev;
DeviceAddress tempDeviceAddress;
float promTemp = 0;

//Variables de almacenamiento en UBIDOTS
#define Lab_Temp "temp"
#define lab_hume "hume"
#define lab_Relay "relay"
#define lab_DS18B20 "Temp_Cabina"
#define lab_CalFil "Filamentos"

//Definicion de tipo de variables a usar e indicacion del pin DHT22
DHT SHT(D0, DHT22);
int H = 0; //Humidity DHT22
float T = 0; //Temperature DHT22
int S_R = 0;
unsigned long tiempAnt;

//Credenciales de conexion a UBIDOTS y la red WIFI
const char* UBIDOTS_TOKEN = "BBFF-myN8ukbnk3L2c307rBTZjZtwkkipQR";  // Put here your Ubidots TOKEN

Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);

void setup() {

  //Conexion Wifi
  connectWifi_Man();
  Serial.println("*************************************");
  Serial.println("Conectado a la red WiFi: " + WiFi.SSID());
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("MacAddress: " + WiFi.macAddress());
  Serial.println("*************************************");

  //Inicializacion DS18B20
  Sensors.begin();
  NumbOfDev = Sensors.getDeviceCount();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(NumbOfDev, DEC);
  Serial.println(" devices.");

  for (int i = 0; i < NumbOfDev; i++) {
    // Search the wire for address
    if (Sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }

  //Inicio sensor DHT22 y declaracion de pines
  SHT.begin();
  pinMode(R_30A, OUTPUT);
  digitalWrite(R_30A, S_R);

  //Prueba para temporizador de envio de Datos
  tiempAnt = millis();
  windowStartTime = millis();

  //inicializacion PID
  Setpoint = 80; //Temperatura de llegada
  myPID.SetOutputLimits(0, WindowSize); //definiendo ventana de tiempo
  myPID.SetMode(AUTOMATIC); // encendiendo PID
}

void loop() {
  S_R = ubidots.get(DEVICE_LABEL, lab_Relay);

  if (S_R != ERROR_VALUE) {
    digitalWrite(R_30A, S_R);
  } else {
    Serial.println("Error activacion");
  }

  S_SSR = ubidots.get(DEVICE_LABEL, lab_CalFil);

  if (S_SSR != ERROR_VALUE) {
    controlPID();
  } else {
    Serial.println("Error activacion");
  }


  Sensors.requestTemperatures();

  if (millis() - tiempAnt >= 2000) {
    tiempAnt = millis();
    for (int i = 0; i < NumbOfDev; i++) {
      // Search the wire for address
      if (Sensors.getAddress(tempDeviceAddress, i)) {
        // Output the device ID
        Serial.print("Temperature for DS18B20: ");
        Serial.println(i, DEC);
        // Print the data
        float tempC = Sensors.getTempC(tempDeviceAddress);
        Serial.print("Temp C: ");
        Serial.println(tempC);
        promTemp = promTemp + tempC;
      }
    }
    promTemp = promTemp / 2;

    H = SHT.readHumidity();
    T = SHT.readTemperature();
    Serial.println("Temp = " + String(T) + " ,Hume = " + String(H));
  }

  ubidots.add(lab_DS18B20, promTemp);
  ubidots.add(lab_hume, H);
  ubidots.add(Lab_Temp, T);
  //ubidots.add(lab_Relay,R_30A);

  bool bufferSent = false;
  bufferSent = ubidots.send("ESP8266-Control");

  if (bufferSent) {
    Serial.println("Values sent by the device");
  }
  promTemp = 0;
  //delay(100);
}
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void controlPID () {
  Input = T;
  myPID.Compute();
  if (millis() - windowStartTime > WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if (Output < millis() - windowStartTime) digitalWrite(SSR, HIGH);
  else digitalWrite(SSR, LOW);  
}
