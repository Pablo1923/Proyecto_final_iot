#include <LoRa.h>
#include "LoRaBoards.h"
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <vector>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "Ai_AP3216_AmbientLightAndProximity.h"

#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           915.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   5
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

using namespace std;

// Variables globales
int counter = 0;
float temperaturaf = 0;
float humidityf = 0;
String estado = "A";
vector<float> temperatures;
vector<float> humidities;

// Pines I2C y GPS
static const int MY_I2C_SDA = 21;
static const int MY_I2C_SCL = 22;
static const int RXPin = 34; // Pin RX para GPS
static const int TXPin = 12;  // Pin TX para GPS

// Configuración del GPS
//SoftwareSerial ss(RXPin, TXPin);
TinyGPSPlus gps;

// Definir WiFi
const char* ssid = "UPBWiFi";
const char* password = "";
const char* host = "10.38.32.137";
const uint16_t port = 1026;
WiFiClient cliente;

// Definir sensor HDC1080 y AP3216
ClosedCube_HDC1080 sensor;
Ai_AP3216_AmbientLightAndProximity aps = Ai_AP3216_AmbientLightAndProximity(); // Configuración correcta del sensor de luz y proximidad

unsigned long lastUpdateTime = 0;

// Prototipos de funciones auxiliares
float averagetemp(const vector<float>& temperatures);
float averagehum(const vector<float>& humidities);
static void smartDelay(unsigned long ms); // Declaración de smartDelay

void setup() {
    // Iniciar comunicación I2C
    Wire.begin(MY_I2C_SDA, MY_I2C_SCL);

    // Configuración del sensor HDC1080
    sensor.begin(0x40);
    delay(20);

    // Iniciar Serial para depuración
    Serial.begin(115200);
    Serial.println("Configurando LoRa, el sensor de luz, el sensor de temperatura y humedad, y GPS...");

    // Configuración del GPS
    Serial2.begin(9600, SERIAL_8N1, 34, 12);

    // Configuración del sensor de luz y proximidad AP3216
    aps.begin();
    aps.startAmbientLightAndProximitySensor();
    Serial.println("Sensor AP3216 inicializado.");

    // Conexión WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado a WiFi");

    // Configuración LoRa
    setupBoards();
    delay(1500);

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
    if (!LoRa.begin(CONFIG_RADIO_FREQ * 1000000)) {
        Serial.println("Error iniciando LoRa");
        while (1);
    }

    LoRa.setTxPower(CONFIG_RADIO_OUTPUT_POWER);
    LoRa.setSignalBandwidth(CONFIG_RADIO_BW * 1000);
    LoRa.setSpreadingFactor(10);
    LoRa.setPreambleLength(16);
    LoRa.setSyncWord(0xAB);
    LoRa.disableCrc();
    LoRa.disableInvertIQ();
    LoRa.setCodingRate4(7);
 
}

void loop() {
    // Captura de datos de temperatura y humedad
  if (estado == "A") {
        for (int i = 0; i < 10; i++) {
            float temperatura = sensor.readTemperature();
            float humedad = sensor.readHumidity();
            humidities.push_back(humedad);
            temperatures.push_back(temperatura);
            smartDelay(100);
        }
        estado = "B";
    }
    
    if (estado == "B"){
      temperaturaf = averagetemp(temperatures);
      humidityf = averagehum(humidities);
      estado = "A";
    }

    // Obtener datos de luz y proximidad
    long luz = aps.getAmbientLight();
    long proximidad = aps.getProximity();

    // Captura de datos GPS
    if (Serial2.available()) {
        gps.encode(Serial2.read());
    }

    // Datos GPS válidos, continuar
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    // Imprimir datos en consola
    Serial.print("Latitud: ");
    Serial.println(latitude, 6);
    Serial.print("Longitud: ");
    Serial.println(longitude, 6);
    Serial.print("Temperatura: ");
    Serial.println(temperaturaf);
    Serial.print("Humedad: ");
    Serial.println(humidityf);
    Serial.print("Luz: ");
    Serial.println(luz);
    Serial.print("Proximidad: ");
    Serial.println(proximidad);

    // Crear JSON para enviar por LoRa
    String postData = String("robocar${\"lat\": {\"value\": ") + String(gps.location.lat(), 6) + ", \"type\": \"Float\"}, " 
             + "\"lon\": {\"value\": " + String(gps.location.lng(), 6) + ", \"type\": \"Float\"}, " 
             + "\"temperatura\": {\"value\": " + String(temperaturaf, 2) + ", \"type\": \"Float\"}, " 
             + "\"humedad\": {\"value\": " + String(humidityf, 2) + ", \"type\": \"Float\"}}";

    // Enviar datos a través de LoRa
    LoRa.beginPacket();
    LoRa.print(postData);
    LoRa.endPacket();

    counter++;
    
    Serial.println("Paquete LoRa enviado: " + postData); // Mostrar datos enviados

    smartDelay(300000);  // Esperar antes del siguiente envío
}

// Funciones auxiliares
float averagetemp(const vector<float>& temperatures) {
    float suma = 0.0;
    for (float temp : temperatures) {
        suma += temp;
    }
    return suma / temperatures.size();
}

float averagehum(const vector<float>& humidities) {
    float suma = 0.0;
    for (float hum : humidities) {
        suma += hum;
    }
    return suma / humidities.size();
}

// Smart delay para GPS
static void smartDelay(unsigned long ms) {
    unsigned long start = millis();
    do {
        while (Serial2.available()) {
            gps.encode(Serial2.read());
        }
    } while (millis() - start < ms);
}
