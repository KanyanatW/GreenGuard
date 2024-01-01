//------------ Blynk-Projekt definieren
#define BLYNK_TEMPLATE_ID "XXX"                     // Blynk Template ID
#define BLYNK_TEMPLATE_NAME "XXX"                     // Blynk Template Name
#define BLYNK_AUTH_TOKEN "XXX"   // Blynk API token

//------------ notwendige Bibliotheken
#include <ESP8266WiFi.h>       // Bibliothek für WIFI
#include <Blynk.h>             // Bibliothek für Blynk
#include <BlynkMultiClient.h>
#include "Arduino.h"
#include <Adafruit_BME680.h>   // Bibliothek für Temperatur und Luftfeuchte Sensoren
#include <Wire.h>              // Bibliothek für Kommunikation mit I2C-Gerät
#include <Adafruit_NeoPixel.h> // Bibliothek für NeoPixel
#include <rgb_lcd.h>           // Bibliothek für LCD 
#include "SI114X.h"            // Bibliothek für Sonnenlicht Sensor      

//------------ Information zu WIFI hier eingeben
String wifiSSID = "XXX";       // Wifi SSID
String wifiPassword = "XXX";   // Wifi Passwort

String matrixausgabe_text = " ";    // Ausgabetext als globale Variable
volatile int matrixausgabe_index = 0; // aktuelle Position in Matrix
IPAddress myOwnIP; // eigene IP für mDNS 
static WiFiClient blynkWiFiClient;
#define BLYNK_PRINT Serial

//------------ Variablen zum Speichern der Messwerte von Sensoren
double temp = 0;
double humid = 0;
int light = 0;
int soil = 0;

int soilPin = 0; // an dem Analog-Pin 0 ist der Erdefeuchte Sensor angeschlossen

//------------ Objekte erstellen
Adafruit_BME680 boschBME680; // Objekt Bosch Umweltsensor 
int boschBME680_ready = 0;   
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, 13, NEO_GRBW + NEO_KHZ800); // Objekt NeoPixel
rgb_lcd lcd;             // Objekt LCD RGB 
SI114X SI1145;           // Objekt SI1145 (Sonnenlicht) Sensor 

//------------ Einmalige Initialisierung
void setup() {

  Serial.begin(115200);
  Wire.begin(); // I2C-Bus initialisieren

  if (Wire.status() != I2C_OK) Serial.println("Etwas stimmt nicht mit I2C");

  // SI1145 Sensor initialisieren 
  Serial.println("Beginne Si1145!");

  while (!SI1145.Begin()) {
    Serial.println("Si1145 ist nicht bereit!");
    delay(1000);
  }
  Serial.println("Si1145 ist bereit!");

  // Bosch BME 680 initialisieren
  boschBME680_ready = boschBME680.begin(118);
  if (boschBME680_ready == 0) {
    while (1) {
      Serial.println("BME680 nicht vorhanden - der alte Octopus nutzt BME280, ggf. Puzzleteile tauschen");
      delay(500);
    }
  }

  // Bosch BME 680 einrichten
  boschBME680.setTemperatureOversampling(BME680_OS_8X);
  boschBME680.setHumidityOversampling(BME680_OS_2X);
  boschBME680.setPressureOversampling(BME680_OS_4X);
  boschBME680.setIIRFilterSize(BME680_FILTER_SIZE_3);
  boschBME680.setGasHeater(0, 0); // aus

  // Neopixel initialisieren
  pixels.begin();
  delay(1);
  pixels.show();
  pixels.setPixelColor(0, 0, 0, 0, 0); // linke LED aus
  pixels.setPixelColor(1, 0, 0, 0, 0); // rechte LED aus
  pixels.show(); // und anzeigen

  // LCD Backlight initialisieren
  lcd.begin(16, 2); // Anzeigetyp einstellen, in diesem Fall 16 Spalten 2 Zeilen

  // WLAN initialisieren 
  Serial.println();
  WiFi.disconnect();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  delay(100);
  Serial.print("\nWLAN verbinden mit:");
  Serial.print(wifiSSID);
  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) { // Warte bis Verbindung steht 
    delay(500);
    Serial.print(".");
  };
  Serial.println("\nverbunden, meine IP:" + WiFi.localIP().toString());
  matrixausgabe_text = " Meine IP:" + WiFi.localIP().toString();
  myOwnIP = WiFi.localIP();
  matrixausgabe_index = 0;

  Blynk.addClient("WiFi", blynkWiFiClient, 80);
  Blynk.config(BLYNK_AUTH_TOKEN);
  int BlynkCon = 0;
  while (BlynkCon == 0) {
    Serial.print("\nBlynk verbinden ... ");
    BlynkCon = Blynk.connect();
    if (BlynkCon == 0) {
      Serial.println("Fehlgeschlagen, versuche es erneut");
      delay(1000);
    }
  }
  Serial.println("verbunden");

}

// Kontinuierliche Wiederholung 
void loop() {

  Blynk.run(); // Blynk Housekeeping

  // Werte von Sensoren lesen
  temp = (boschBME680.readTemperature() + (0.0));
  humid = (boschBME680.readHumidity() + (0.0));
  soil = analogRead(soilPin);
  light = (SI1145.ReadVisible());

  // Werte an Blynk-Server übermitteln
  Blynk.virtualWrite(0, temp);
  Blynk.run(); // Blynk Housekeeping
  Blynk.virtualWrite(1, humid);
  Blynk.run(); // Blynk Housekeeping
  Blynk.virtualWrite(2, soil);
  Blynk.run(); // Blynk Housekeeping
  Blynk.virtualWrite(3, light);
  Blynk.run(); // Blynk Housekeeping

  // Werte auf LCD anzeigen lassen(Nummerierung beginnt bei Null)
  lcd.setCursor(0, 0); // Spalte 0, Zeile 0
  lcd.print(String("T:" + String(temp)));
  lcd.setCursor(8, 0); // Spalte 8, Zeile 0                                                 
  lcd.print(String("H:" + String(humid)));
  lcd.setCursor(0, 1); // Spalte 0, Zeile 1
  lcd.print(String("S:" + String(soil)));
  lcd.setCursor(8, 1); // Spalte 8, Zeile 1
  lcd.print(String("L:" + String(light)));

  // Werte auf Serieller Monitor ausgeben
  Serial.print("Temperatur:" + String(temp));
  Serial.println();
  Serial.print("Luftfeuchtigkeit:" + String(humid));
  Serial.println();
  Serial.print("Erdefeuchte:" + String(soil));
  Serial.println();
  Serial.print("Sichtbares Licht:" + String(light));
  Serial.println();

  // Beginn der Abfrage
  if (((light) < (400)) || ((light) > (700))) // Wenn Sonnenlicht-Wert < 400 oder > 700 ist,
  {
    pixels.setPixelColor(0, 40, 0, 0, 0); // dann beleuchtet linke LED in Rot
    pixels.show();
    delay(500);
    pixels.setPixelColor(0, 0, 0, 0, 0);
    pixels.show();
    delay(500);
  }
  else {
    pixels.setPixelColor(0, 0, 40, 0, 0); // ansonsten bleibt linke LED in Grün
    pixels.show();
  }

  if (((soil) < (190)) || ((soil) > (570))) // Wenn Erdefeuchte-Wert < 190 oder > 570 ist,
  {
    pixels.setPixelColor(1, 40, 0, 0, 0); // dann beleuchtet rechte LED in Rot
    pixels.show();
    delay(500);
    pixels.setPixelColor(1, 0, 0, 0, 0);
    pixels.show();
    delay(500);
  }
  else {
    pixels.setPixelColor(1, 0, 40, 0, 0); // ansonsten bleibt linke LED in Grün
    pixels.show();
  }

  // 1000 Millisekunden bis zur Wiederholung warten
  delay(1000);

} // end loop
