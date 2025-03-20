#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Seeed_TMG3993.h"

// Define SPI pins for BME680
#define BME_SCK 36
#define BME_MISO 37
#define BME_MOSI 35
#define BME_CS 34

// Define sensor objects
Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // BME680 in SPI mode
TMG3993 tmg3993; // TMG3993 in I2C mode

#define SEALEVELPRESSURE_HPA (1013.25)

// Wi-Fi credentials
const char *ssid = "EmmaD";
const char *password = "coucoutoi";

// Web server on port 80
WiFiServer server(80);

// Timing variables
unsigned long previousMillis = 0; // Stores the time of the last reading
const long interval = 5000;       // Reading interval: 5 seconds
String lastReadingTime = "No data yet"; // Stores the time of the last sensor reading

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Initializing sensors and connecting to Wi-Fi...");

    // Initialize BME680 in SPI mode
    if (!bme.begin()) {
        Serial.println("Error: BME680 not detected. Check SPI wiring!");
        while (1);
    }

    // Configure BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C for 150 ms

    // Initialize TMG3993 in I2C mode
    Wire.begin();
    if (tmg3993.initialize() == false) {
        Serial.println("Error: TMG3993 not detected. Check I2C wiring!");
        while (1);
    }

    // Configure TMG3993
    tmg3993.setADCIntegrationTime(0xdb); // Integration time: 103 ms
    tmg3993.enableEngines(ENABLE_PON | ENABLE_AEN | ENABLE_AIEN);

    // Connect to Wi-Fi
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start the web server
    server.begin();
    Serial.println("Web server started.");
}

void loop() {
    // Check for client connections
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected.");
        String currentLine = "";

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);

                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        // Send HTTP response
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // Display sensor data
                        client.println("<html><body>");
                        client.println("<h1>Sensor Data</h1>");

                        // Display last reading time
                        client.print("<p>Last sensor reading: ");
                        client.print(lastReadingTime);
                        client.println("</p>");

                        // Refresh button
                        client.println("<button onclick=\"window.location.reload();\">Refresh</button>");
                        client.println("<br><br>");

                        // Read BME680 data
                        if (!bme.performReading()) {
                            client.println("<p>Error: BME680 reading failed!</p>");
                        } else {
                            client.println("<h2>BME680</h2>");
                            client.print("<p>Temperature = ");
                            client.print(bme.temperature);
                            client.println(" *C</p>");

                            client.print("<p>Pressure = ");
                            client.print(bme.pressure / 100.0);
                            client.println(" hPa</p>");

                            client.print("<p>Humidity = ");
                            client.print(bme.humidity);
                            client.println(" %</p>");

                            client.print("<p>Gas resistance = ");
                            client.print(bme.gas_resistance / 1000.0);
                            client.println(" KOhms</p>");

                            client.print("<p>Approximate altitude = ");
                            client.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
                            client.println(" m</p>");
                        }

                        // Read TMG3993 data
                        if (tmg3993.getSTATUS() & STATUS_AVALID) {
                            uint16_t r, g, b, c;
                            int32_t lux;// cct;
                            tmg3993.getRGBCRaw(&r, &g, &b, &c);
                            lux = tmg3993.getLux(r, g, b, c);
                            //cct = tmg3993.getCCT(r, g, b, c);

                            client.println("<h2>TMG3993</h2>");
                            /*client.print("<p>RGBC data: ");
                            client.print(r);
                            client.print(", ");
                            client.print(g);
                            client.print(", ");
                            client.print(b);
                            client.print(", ");
                            client.print(c);
                            client.println("</p>");*/

                            client.print("<p>Lux = ");
                            client.print(lux);
                            client.print("</p>");
                            /*
                            client.print("<p>CCT = ");
                            client.print(cct);
                            client.println("</p>");*/

                            // Clear TMG3993 interrupts
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

        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
    }

    // Read sensors every 5 seconds
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; // Update the time of the last reading

        // Update the last reading time
        lastReadingTime = String(currentMillis / 1000) + " seconds since startup";

        // Read sensors
        if (!bme.performReading()) {
            Serial.println("Error: BME680 reading failed!");
        } else {
            Serial.println("---- BME680 Data ----");
            Serial.print("Temperature = ");
            Serial.print(bme.temperature);
            Serial.println(" *C");

            Serial.print("Pressure = ");
            Serial.print(bme.pressure / 100.0);
            Serial.println(" hPa");

            Serial.print("Humidity = ");
            Serial.print(bme.humidity);
            Serial.println(" %");

            Serial.print("Gas resistance = ");
            Serial.print(bme.gas_resistance / 1000.0);
            Serial.println(" KOhms");

            Serial.print("Approximate altitude = ");
            Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
            Serial.println(" m");
        }

        if (tmg3993.getSTATUS() & STATUS_AVALID) {
            uint16_t r, g, b, c;
            int32_t lux;
            tmg3993.getRGBCRaw(&r, &g, &b, &c);
            lux = tmg3993.getLux(r, g, b, c);

            Serial.println("---- TMG3993 Data ----");

            Serial.print("Lux = ");
            Serial.print(lux);

            // Clear TMG3993 interrupts
            tmg3993.clearALSInterrupts();
        }
    }
}
