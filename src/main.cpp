#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <FastLED.h>
#include <ArduinoOTA.h>

// -------- LED configuration -------- //
#define DATA_PIN       23               // GPIO23 drives the LED strip
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define LEDS_PER_STRIP 17
#define MAX_STRIPS     30               // Maximum number of strips supported
#define MAX_LEDS       (LEDS_PER_STRIP * MAX_STRIPS)

CRGB leds[MAX_LEDS];

// -------- Timing configuration -------- //
uint16_t delayPerLED = 100;               // 8 ms delay per LED

// -------- Global state -------- //
bool ledRunning = true;                // Toggle for LED effect
int numStrips = MAX_STRIPS;             // Default: 30 strips
int currentLEDCount = LEDS_PER_STRIP * 30; // Actual number of LEDs to drive
int currentLED = 0;                     // Current LED index for the running effect
unsigned long previousMillis = 0;       // For non-blocking timing

// -------- WiFi credentials -------- //
const char* ssid = "Perseverance";
const char* password = "AMeeZu5o?wifi";

// -------- Web server -------- //
WebServer server(80);

// -------- HTML page -------- //
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 LED Control</title></head><body>";
  html += "<h1>ESP32 LED Control</h1>";
  html += "<p>LED effect is currently <strong>" + String(ledRunning ? "RUNNING" : "STOPPED") + "</strong>.</p>";
  html += "<p><a href=\"/toggle\">Toggle LED Effect</a></p>";
  html += "<hr>";
  html += "<h2>Set Number of Strips</h2>";
  html += "<form action=\"/setStrips\" method=\"POST\">";
  html += "Number of strips (1-" + String(MAX_STRIPS) + "): <input type=\"number\" name=\"strips\" min=\"1\" max=\"" + String(MAX_STRIPS) + "\" value=\"" + String(numStrips) + "\">";
  html += "<input type=\"submit\" value=\"Set\">";
  html += "</form>";
  html += "<hr>";
  html += "</body></html>";
  return html;
}

// -------- HTTP Handlers -------- //
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleToggle() {
  ledRunning = !ledRunning;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetStrips() {
  if (server.hasArg("strips")) {
    int strips = server.arg("strips").toInt();
    if (strips >= 1 && strips <= MAX_STRIPS) {
      numStrips = strips;
      currentLEDCount = LEDS_PER_STRIP * numStrips;
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}


void setup() {
  // Initialize Serial monitor
  Serial.begin(115200);
  delay(1000);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, MAX_LEDS);
  FastLED.clear();
  FastLED.show();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // ----- Initialize ArduinoOTA ----- //
  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA update");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Configure web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/setStrips", HTTP_POST, handleSetStrips);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  // Handle OTA update requests (for PlatformIO OTA)
  ArduinoOTA.handle();

  // Handle web server requests
  server.handleClient();

  // Run LED effect if enabled
  if (ledRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= delayPerLED) {
      previousMillis = currentMillis;
      FastLED.clear();
      if (currentLED < currentLEDCount) {
        leds[currentLED] = CRGB::Red;
        currentLED++;
      } else {
        currentLED = 0;
      }
      FastLED.show();
    }
  } else {
    // Optionally, clear the LED strip when not running.
    FastLED.clear();
    FastLED.show();
  }
}
