#include <LoRa.h>
#include "LoRaBoards.h"
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <vector>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           915.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

#if !defined(USING_SX1276) && !defined(USING_SX1278)
#error "LoRa example is only allowed to run SX1276/78. For other RF models, please run examples/RadioLibExamples"
#endif

// Variables globales
int counter = 0;
float temperaturaf = 0;
float humidityf = 0;
float lightLevel = 0;
String estado = "A";
vector<float> temperatures;
vector<float> humidities;

// Pines I2C y GPS
static const int I2C_SDA = 21; // SDA y SCL
static const int I2C_SCL = 22;
static const int RXPin = 34, TXPin = 12;

// Configuración del GPS
SoftwareSerial ss(RXPin, TXPin);
TinyGPSPlus gps;

// Definir WiFi
const char* ssid = "UPBWiFi";
const char* password = "";
WiFiClient cliente;

// Definir sensores
ClosedCube_HDC1080 sensor;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345); // Dirección por defecto

// ID del dispositivo
const char* deviceId = "point10"; // ID para David Zapata

unsigned long lastUpdateTime = 0;

void setup() {
    // Iniciar comunicación I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // Configuración del sensor HDC1080
    sensor.begin(0x40);
    delay(20);

    // Iniciar Serial para depuración
    Serial.begin(115200);
    Serial.println("Configurando LoRa, el sensor y GPS...");

    // Configuración del GPS
    ss.begin(9600);

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
        Serial.println("Starting LoRa failed!");
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

    // Configuración del sensor de luz
    if (!tsl.begin()) {
        Serial.println("No se pudo encontrar el sensor de luz TSL2561.");
    } else {
        tsl.enableAutoRange(true);  // Ajuste automático del rango
        tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); // Tiempo de integración
        Serial.println("Sensor de luz TSL2561 configurado.");
    }
}

void loop() {
    // Captura de datos de temperatura y humedad
    if (estado == "A") {
        for (int i = 0; i < 10; i++) {
            float temperatura = sensor.readTemperature();
            float humedad = sensor.readHumidity();
            humidities.push_back(humedad);
            temperatures.push_back(temperatura);
            delay(100);
        }
        temperaturaf = averagetemp(temperatures);
        humidityf = averagehum(humidities);
        estado = "B";
    }

    // Captura de datos de luz
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light) {
        lightLevel = event.light;
    } else {
        lightLevel = 0.0;
        Serial.println("No se pudo leer el nivel de luz.");
    }

    if (ss.available()) {
        gps.encode(ss.read());
    }

    if (gps.location.isValid()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();

        // Imprimir en consola
        Serial.print("Latitud: ");
        Serial.println(latitude, 6);
        Serial.print("Longitud: ");
        Serial.println(longitude, 6);
        Serial.print("Temperatura: ");
        Serial.println(temperaturaf);
        Serial.print("Humedad: ");
        Serial.println(humidityf);
        Serial.print("Nivel de luz: ");
        Serial.println(lightLevel);

        // Crear JSON para enviar por LoRa
        String postData = String("{\"id\": \"") + deviceId + "\"," 
                        + "\"lat\": " + String(latitude, 6) + ", " 
                        + "\"lon\": " + String(longitude, 6) + ", " 
                        + "\"temperatura\": " + String(temperaturaf, 2) + ", " 
                        + "\"humedad\": " + String(humidityf, 2) + ", "
                        + "\"luz\": " + String(lightLevel, 2) + "}";

        // Enviar datos a través de LoRa
        LoRa.beginPacket();
        LoRa.print(postData);
        LoRa.endPacket();

        counter++;
    }

    delay(5000);  // Esperar antes del siguiente envío
}

// Funciones auxiliares
float averagetemp(const vector<float> &temperatures) {
    float suma = 0.0;
    for (float temp : temperatures) {
        suma += temp;
    }
    return suma / temperatures.size();
}

float averagehum(const vector<float> &humidities) {
    float suma = 0.0;
    for (float hum : humidities) {
        suma += hum;
    }
    return suma / humidities.size();
}
