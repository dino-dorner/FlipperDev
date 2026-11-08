
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ================= CONFIGURATION =================

// WiFi
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Finnhub API
const char* FINNHUB_TOKEN = "YOUR_FINNHUB_API_KEY";

// UART
#define UART_RX 16
#define UART_TX 17
#define BAUD_RATE 115200

HardwareSerial FlipperSerial(2);

// Maximum supported tickers
#define MAX_TICKERS 32
#define TICKER_LENGTH 16

// =================================================

String tickers[MAX_TICKERS];
int tickerCount = 0;

// Connect ESP32 to WiFi
void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());
}

// Validate ticker input
bool validTicker(String ticker) {
    ticker.trim();

    if (ticker.length() == 0 ||
        ticker.length() >= TICKER_LENGTH) {
        return false;
    }

    for (int i = 0; i < ticker.length(); i++) {
        char c = ticker[i];

        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '-')) {
            return false;
        }
    }

    return true;
}

// Receive ticker list from Flipper
bool receiveTickers() {
    tickerCount = 0;

    Serial.println("Waiting for ticker list...");

    // Wait for TICKERS command
    String command = "";

    unsigned long start = millis();

    while (millis() - start < 10000) {
        if (FlipperSerial.available()) {
            command = FlipperSerial.readStringUntil('\n');
            command.trim();

            if (command == "TICKERS") {
                break;
            }
        }
    }

    if (command != "TICKERS") {
        return false;
    }

    // Receive each ticker until END
    while (tickerCount < MAX_TICKERS) {
        if (!FlipperSerial.available()) {
            continue;
        }

        String ticker = FlipperSerial.readStringUntil('\n');
        ticker.trim();

        if (ticker == "END") {
            return true;
        }

        if (!validTicker(ticker)) {
            continue;
        }

        ticker.toUpperCase();

        // Prevent duplicates
        bool duplicate = false;

        for (int i = 0; i < tickerCount; i++) {
            if (tickers[i] == ticker) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            tickers[tickerCount++] = ticker;
        }
    }

    return true;
}

// Query Finnhub for a stock quote
bool getStockQuote(
    String ticker,
    float &price,
    float &change
) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure client;

    // Development/testing only.
    // Use certificate validation in production.
    client.setInsecure();

    HTTPClient http;

    String url =
        "https://finnhub.io/api/v1/quote?symbol=" +
        ticker +
        "&token=" +
        FINNHUB_TOKEN;

    if (!http.begin(client, url)) {
        return false;
    }

    http.setTimeout(10000);

    int responseCode = http.GET();

    if (responseCode != HTTP_CODE_OK) {
        Serial.printf(
            "API error for %s: %d\n",
            ticker.c_str(),
            responseCode
        );

        http.end();
        return false;
    }

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(doc, http.getString());

    http.end();

    if (error) {
        Serial.println("JSON parsing failed");
        return false;
    }

    float currentPrice = doc["c"] | 0.0f;
    float previousClose = doc["pc"] | 0.0f;
    float percentageChange = doc["dp"] | 0.0f;

    // Finnhub returns zero values when quote data
    // is unavailable for a symbol.
    if (currentPrice <= 0 || previousClose <= 0) {
        return false;
    }

    price = currentPrice;
    change = percentageChange;

    return true;
}

// Send results back to Flipper
void sendStockResults() {
    FlipperSerial.println("DATA");

    for (int i = 0; i < tickerCount; i++) {

        String ticker = tickers[i];

        float price = 0;
        float change = 0;

        Serial.printf("Fetching %s...\n", ticker.c_str());

        bool success = getStockQuote(
            ticker,
            price,
            change
        );

        if (success) {
            // Format:
            // TICKER,PRICE,PERCENT_CHANGE

            FlipperSerial.printf(
                "%s,%.2f,%.2f\n",
                ticker.c_str(),
                price,
                change
            );

            Serial.printf(
                "%s: $%.2f (%.2f%%)\n",
                ticker.c_str(),
                price,
                change
            );
        }
        else {
            // Format:
            // TICKER,ERR

            FlipperSerial.printf(
                "%s,ERR\n",
                ticker.c_str()
            );
        }

        // Avoid sending API requests too quickly.
        delay(250);
    }

    FlipperSerial.println("END");

    Serial.println("Finished updating stocks!");
}

// ================= SETUP =================

void setup() {
    Serial.begin(115200);

    FlipperSerial.begin(
        BAUD_RATE,
        SERIAL_8N1,
        UART_RX,
        UART_TX
    );

    connectWiFi();

    Serial.println("Stock bridge ready!");
}

// ================= MAIN LOOP =================

void loop() {

    if (FlipperSerial.available()) {

        if (receiveTickers()) {

            Serial.printf(
                "Received %d tickers\n",
                tickerCount
            );

            sendStockResults();
        }
    }

    delay(10);
}
