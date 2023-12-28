#define BLYNK_TEMPLATE_ID "TMPL4I0CMO-KM"
#define BLYNK_TEMPLATE_NAME "Green Guard"
#define BLYNK_AUTH_TOKEN "YNcvkNthcS0zyzYgOc-1tktl-Ei4wh9G"

/* Disclaimer IoT-Werkstatt CC 4.0 BY NC SA 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For Ardublock see the
 GNU General Public License for more details. */
#include <ESP8266WiFi.h>
#include <Blynk.h>
#include <BlynkMultiClient.h>
#include <Adafruit_BME680.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <rgb_lcd.h>
#include "SI114X.h"         


String matrixausgabe_text  = " "; // Ausgabetext als globale Variable

volatile int matrixausgabe_index = 0;// aktuelle Position in Matrix

IPAddress myOwnIP; // ownIP for mDNS 

static WiFiClient blynkWiFiClient;
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
double temp = 0 ;
// BME680 Lib written by Limor Fried & Kevin Townsend for Adafruit Industries, http://www.adafruit.com/products/3660
Adafruit_BME680 boschBME680; // Objekt Bosch Umweltsensor
int boschBME680_ready = 0;

double humid = 0 ;
int light = 0;    // range 280-950 aber gut 400-700
int soil = 0 ;    // range 0-950 aber gut 190-570

/* converted variables
int lightPercent;
int soilPercent;
*/

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2,13,NEO_GRBW + NEO_KHZ800);
//LCD RGB, 2013 Copyright (c) Seeed Technology Inc.   Author:Loovee
rgb_lcd lcd;

SI114X SI1145 = SI114X(); // Create an instance of the SI1145 sensor

void setup(){ // Einmalige Initialisierung
  Serial.begin(115200);
  Wire.begin(); // ---- Initialisiere den I2C-Bus 

  if (Wire.status() != I2C_OK) Serial.println("Something wrong with I2C");

  boschBME680_ready = boschBME680.begin(118);

  if (boschBME680_ready == 0) {
    while(1) { 
      Serial.println("BME680 nicht vorhanden - der alte Octopus nutzt BME280, ggf. Puzzleteile tauschen");
      delay(500);
    }
  }

  // Set up Bosch BME 680
  boschBME680.setTemperatureOversampling(BME680_OS_8X);
  boschBME680.setHumidityOversampling(BME680_OS_2X);
  boschBME680.setPressureOversampling(BME680_OS_4X);
  boschBME680.setIIRFilterSize(BME680_FILTER_SIZE_3);
  boschBME680.setGasHeater(0, 0); // off

  pixels.begin();//-------------- Initialisierung Neopixel
  delay(1);
  pixels.show();
  pixels.setPixelColor(0,0,0,0,0); // alle aus
  pixels.setPixelColor(1,0,0,0,0);
  pixels.show();                 // und anzeigen

  lcd.begin(16, 2);// LCD Backlight initialisieren 

  Serial.println();
  //------------ WLAN initialisieren 
  WiFi.disconnect();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  delay(100);
  Serial.print ("\nWLAN connect to:");
  Serial.print("wifi-name");
  WiFi.begin("wifi-name","wifi-password");
  while (WiFi.status() != WL_CONNECTED) { // Warte bis Verbindung steht 
    delay(500); 
    Serial.print(".");
  };
  Serial.println ("\nconnected, meine IP:"+ WiFi.localIP().toString());
  matrixausgabe_text = " Meine IP:" + WiFi.localIP().toString();
  myOwnIP = WiFi.localIP();
  matrixausgabe_index=0;

  Blynk.addClient("WiFi", blynkWiFiClient, 80);
  Blynk.config(BLYNK_AUTH_TOKEN); // Downloads, docs, tutorials: http://www.blynk.cc
  int BlynkCon = 0;
  while (BlynkCon == 0) {
    Serial.print ("\nBlynk connect ... ");
    BlynkCon=Blynk.connect();
    if (BlynkCon == 0) {
      Serial.println("failed, try again");
      delay(1000);
    }
  }
  Serial.println("connected");

}

void loop() { // Kontinuierliche Wiederholung 
  Blynk.run();// Blynk Housekeeping
  temp = (boschBME680.readTemperature()+(0.0)) ;
  humid = (boschBME680.readHumidity()+(0.0)) ;
  soil = analogRead(0) ;
  light = SI1145.ReadVisible();

  /*map 
  soilPercent = map(soil,0,950,0,100);
  lightPercent = map(light,280,950,0,100);
  */
  
  Blynk.virtualWrite(0,temp);// Wert an Blynk-Server übermitteln
  Blynk.run();// Blynk Housekeeping
  Blynk.virtualWrite(1,humid);// Wert an Blynk-Server übermitteln
  Blynk.run();// Blynk Housekeeping
  Blynk.virtualWrite(2,soil);// Wert an Blynk-Server übermitteln
  Blynk.run();// Blynk Housekeeping
  Blynk.virtualWrite(3,light);// Wert an Blynk-Server übermitteln
  Blynk.run();// Blynk Housekeeping

  lcd.setCursor(0,0);                                                  // Zeile 1, ab 0
  lcd.print(String("T:"+String(temp)));       
  lcd.setCursor(8,0);                                                   // Zeile 1, ab 8
  lcd.print(String("H:"+String(humid)));
  lcd.setCursor(0,1);                                                   // Zeile 2, ab 0
  lcd.print(String("S:"+String(soil)));
  lcd.setCursor(8,1);                                                   // Zeile 2, ab 8
  lcd.print(String("V:"+String(light)));

  // Print data to Serial Monitor
  Serial.print("Temperature:"+String(temp));
  Serial.println();
  Serial.print("Humidity:"+String(humid));
  Serial.println();
  Serial.print("Soil Moisture:"+String(soil));
  Serial.println();
  Serial.print("Visible Light:"+String(light));
  Serial.println();

  // wenn light/soil unter bzw. oberhalb diese Werte liegt, wird rot beleuchtet
  if (( ( light ) < ( 400 ) ) || ( ( light ) > ( 700 ) ))   // nur dann wird beleuchtet
  {
    pixels.setPixelColor(0,40,0,0,0);       // right light
    pixels.show();
    delay(500);
    pixels.setPixelColor(0,0,0,0,0);        
    pixels.show();
    delay(500);
  }
  
  if (( ( soil ) < ( 190 ) ) || ( ( soil ) > ( 570 ) ))     // nur dann wird beleuchtet
  {

    pixels.setPixelColor(1,40,0,0,0);       // left light
    pixels.show();
    delay(500);
    pixels.setPixelColor(1,0,0,0,0);        
    pixels.show();
    delay(500);
  }
  
  delay( 1000 );
} //end loop
