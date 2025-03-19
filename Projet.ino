#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Seeed_TMG3993.h"

// Définir les broches SPI pour le BME680
#define BME_SCK 36
#define BME_MISO 37
#define BME_MOSI 35
#define BME_CS 34

// Définir les objets pour les capteurs
Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // BME680 en mode SPI
TMG3993 tmg3993; // TMG3993 en mode I2C

#define SEALEVELPRESSURE_HPA (1013.25)

// Paramètres Wi-Fi
const char *ssid = "EmmaD";
const char *password = "coucoutoi";

// Serveur web sur le port 80
WiFiServer server(80);

// Variables pour le timing
unsigned long previousMillis = 0; // Stocke le temps de la dernière lecture
const long interval = 5000;       // Intervalle de lecture : 5 secondes

void setup() {
    // Initialisation de la communication série
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Initialisation des capteurs et connexion Wi-Fi...");

    // Initialisation du BME680 en mode SPI
    if (!bme.begin()) {
        Serial.println("Erreur : BME680 non détecté. Vérifiez le câblage SPI !");
        while (1);
    }

    // Configuration du BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C pendant 150 ms

    // Initialisation du TMG3993 en mode I2C
    Wire.begin();
    if (tmg3993.initialize() == false) {
        Serial.println("Erreur : TMG3993 non détecté. Vérifiez le câblage I2C !");
        while (1);
    }

    // Configuration du TMG3993
    tmg3993.setADCIntegrationTime(0xdb); // Temps d'intégration : 103 ms
    tmg3993.enableEngines(ENABLE_PON | ENABLE_AEN | ENABLE_AIEN);

    // Connexion au Wi-Fi
    Serial.print("Connexion au Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnecté au Wi-Fi !");
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());

    // Démarrer le serveur web
    server.begin();
    Serial.println("Serveur web démarré.");
}

void loop() {
    // Vérifier les connexions des clients
    WiFiClient client = server.available();
    if (client) {
        Serial.println("Nouveau client connecté.");
        String currentLine = "";

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);

                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        // Envoyer la réponse HTTP
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // Afficher les données des capteurs
                        client.println("<html><body>");
                        client.println("<h1>Donnees des capteurs</h1>");

                        // Bouton de rafraîchissement
                        client.println("<button onclick=\"window.location.reload();\">Rafraichir</button>");
                        client.println("<br><br>");

                        // Lecture des données du BME680
                        if (!bme.performReading()) {
                            client.println("<p>Erreur : Lecture du BME680 echouee !</p>");
                        } else {
                            client.println("<h2>BME680</h2>");
                            client.print("<p>Temperature = ");
                            client.print(bme.temperature);
                            client.println(" *C</p>");

                            client.print("<p>Pression = ");
                            client.print(bme.pressure / 100.0);
                            client.println(" hPa</p>");

                            client.print("<p>Humidite = ");
                            client.print(bme.humidity);
                            client.println(" %</p>");

                            client.print("<p>Resistance du gaz = ");
                            client.print(bme.gas_resistance / 1000.0);
                            client.println(" KOhms</p>");

                            client.print("<p>Altitude approximative = ");
                            client.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
                            client.println(" m</p>");
                        }

                        // Lecture des données du TMG3993
                        if (tmg3993.getSTATUS() & STATUS_AVALID) {
                            uint16_t r, g, b, c;
                            int32_t lux, cct;
                            tmg3993.getRGBCRaw(&r, &g, &b, &c);
                            lux = tmg3993.getLux(r, g, b, c);
                            cct = tmg3993.getCCT(r, g, b, c);

                            client.println("<h2>TMG3993</h2>");
                            client.print("<p>Donnees RGBC : ");
                            client.print(r);
                            client.print(", ");
                            client.print(g);
                            client.print(", ");
                            client.print(b);
                            client.print(", ");
                            client.print(c);
                            client.println("</p>");

                            client.print("<p>Lux = ");
                            client.print(lux);
                            client.print("</p>");

                            client.print("<p>CCT = ");
                            client.print(cct);
                            client.println("</p>");

                            // Effacer les interruptions du TMG3993
                            tmg3993.clearALSInterrupts();
                        }

                        client.println("</body></html>");
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }

        // Fermer la connexion
        client.stop();
        Serial.println("Client déconnecté.");
    }

    // Lecture des capteurs toutes les 5 secondes
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; // Mettre à jour le temps de la dernière lecture

        // Lecture des capteurs
        if (!bme.performReading()) {
            Serial.println("Erreur : Lecture du BME680 echouee !");
        } else {
            Serial.println("---- Donnees du BME680 ----");
            Serial.print("Temperature = ");
            Serial.print(bme.temperature);
            Serial.println(" *C");

            Serial.print("Pression = ");
            Serial.print(bme.pressure / 100.0);
            Serial.println(" hPa");

            Serial.print("Humidite = ");
            Serial.print(bme.humidity);
            Serial.println(" %");

            Serial.print("Resistance du gaz = ");
            Serial.print(bme.gas_resistance / 1000.0);
            Serial.println(" KOhms");

            Serial.print("Altitude approximative = ");
            Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
            Serial.println(" m");
        }

        if (tmg3993.getSTATUS() & STATUS_AVALID) {
            uint16_t r, g, b, c;
            int32_t lux, cct;
            tmg3993.getRGBCRaw(&r, &g, &b, &c);
            lux = tmg3993.getLux(r, g, b, c);
            cct = tmg3993.getCCT(r, g, b, c);

            Serial.println("---- Donnees du TMG3993 ----");
            Serial.print("Donnees RGBC : ");
            Serial.print(r);
            Serial.print(", ");
            Serial.print(g);
            Serial.print(", ");
            Serial.print(b);
            Serial.print(", ");
            Serial.println(c);

            Serial.print("Lux = ");
            Serial.print(lux);
            Serial.print(", CCT = ");
            Serial.println(cct);

            // Effacer les interruptions du TMG3993
            tmg3993.clearALSInterrupts();
        }
    }
}
