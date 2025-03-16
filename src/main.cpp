#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <FastLED.h>
#include <ArduinoOTA.h>

// -------- LED configuration -------- //
#define DATA_PIN       23               // GPIO pin for LED data
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define LEDS_PER_STRIP 17
#define MAX_STRIPS     30               // Maximum number of strips supported
#define MAX_LEDS       (LEDS_PER_STRIP * MAX_STRIPS)

CRGB leds[MAX_LEDS];

// -------- Global state and pattern parameters -------- //
bool ledRunning = true;                // Toggle for LED effect
int numStrips = MAX_STRIPS;            // Default: maximum strips
int currentLEDCount = LEDS_PER_STRIP * MAX_STRIPS; // Actual number of LEDs

// Pattern parameter defaults (settable via the webpage)
int Speed = 50;         // (0-255)
int Hue = 0;            // (0-255)
int Saturation = 255;   // (0-255)
int Brightness = 128;   // (0-255)

int PatMode = 0;        // Pattern mode (0 to 6)
int ColMode = 0;        // Colour mode (0: fixed, 1: rainbow)

const int NumPatternModes = 7;
const int NumColourModes = 2;

// -------- Variables for additional patterns -------- //

// For COMETS (PatternMode2)
#define NumComets 20
int CometDirection[NumComets];
int CometStart[NumComets];
int CometCounter[NumComets];
int CometLED[NumComets];
int CometColour[NumComets];

// For FIREWORKS (PatternMode3)
#define NumVertices 20
#define VertexConnections 3
int FireVertex = 0;
int FireCounter = 0;
int FireSize = 0;
int FireDelay = 0;
int FireColour = 0;
int FireDirection[VertexConnections];
int FireStart[VertexConnections];
int FireLED[VertexConnections];

// For MARQUEE (PatternMode4)
int t = 0;
int Marquee = 1;

// For PACMAN (PatternMode5)
int Game = 0;
int PacManDirection[5];
int PacManStart[5];
int PacManCounter[5];
int PacManLED[5];
int PacManColour[5] = {40, 0, 19, 135, 220};

// For CHRISTMAS (PatternMode6)
int ChristmasColour[2][3] = {
  {0, 96, 0},
  {255, 255, 0}
};

// Vertex array for FIREWORKS, PACMAN, etc.
// Each row contains three vertices (connections) for a given vertex.
int VertexArrayData[NumVertices][VertexConnections] = {
  {84,  85,   0},  // A
  {16,  17, 152},  // B
  {101,102,339},  // C
  {67,  68, 305},  // D
  {50,  51, 254},  // E
  {33,  34, 203},  // F
  {135,136,153},  // G
  {118,119,390},  // H
  {322,323,340},  // I
  {288,289,306},  // J
  {271,272,492},  // K
  {237,238,255},  // L
  {186,187,204},  // M
  {169,170,424},  // N
  {373,374,391},  // O
  {356,357,509},  // P
  {475,476,493},  // Q
  {220,221,458},  // R
  {407,408,425},  // S
  {441,442,459}   // T
};

// -------- Web server -------- //
WebServer server(80);

// -------- HTML page -------- //
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 LED Control</title></head><body>";
  html += "<h1>ESP32 LED Control</h1>";
  html += "<p>LED effect is currently <strong>" + String(ledRunning ? "RUNNING" : "STOPPED") + "</strong>.</p>";
  html += "<p><a href=\"/toggle\">Toggle LED Effect</a></p>";
  html += "<hr>";
  
  // LED configuration form
  html += "<h2>LED Configuration</h2>";
  html += "<form action=\"/setStrips\" method=\"POST\">";
  html += "Number of strips (1-" + String(MAX_STRIPS) + "): <input type=\"number\" name=\"strips\" min=\"1\" max=\"" + String(MAX_STRIPS) + "\" value=\"" + String(numStrips) + "\">";
  html += "<input type=\"submit\" value=\"Set\">";
  html += "</form>";
  
  html += "<hr>";
  
  // Pattern parameters form
  html += "<h2>Pattern Parameters</h2>";
  html += "<form action=\"/setParams\" method=\"POST\">";
  html += "Pattern Mode (0-" + String(NumPatternModes - 1) + "): <input type=\"number\" name=\"patMode\" min=\"0\" max=\"" + String(NumPatternModes - 1) + "\" value=\"" + String(PatMode) + "\"><br>";
  html += "Colour Mode (0-" + String(NumColourModes - 1) + "): <input type=\"number\" name=\"colMode\" min=\"0\" max=\"" + String(NumColourModes - 1) + "\" value=\"" + String(ColMode) + "\"><br>";
  html += "Speed (0-255): <input type=\"number\" name=\"speed\" min=\"0\" max=\"255\" value=\"" + String(Speed) + "\"><br>";
  html += "Hue (0-255): <input type=\"number\" name=\"hue\" min=\"0\" max=\"255\" value=\"" + String(Hue) + "\"><br>";
  html += "Saturation (0-255): <input type=\"number\" name=\"saturation\" min=\"0\" max=\"255\" value=\"" + String(Saturation) + "\"><br>";
  html += "Brightness (0-255): <input type=\"number\" name=\"brightness\" min=\"0\" max=\"255\" value=\"" + String(Brightness) + "\"><br>";
  html += "<input type=\"submit\" value=\"Set Parameters\">";
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

void handleSetParams() {
  if (server.hasArg("patMode")) {
    PatMode = server.arg("patMode").toInt();
    if (PatMode < 0) PatMode = 0;
    if (PatMode >= NumPatternModes) PatMode = NumPatternModes - 1;
  }
  if (server.hasArg("colMode")) {
    ColMode = server.arg("colMode").toInt();
    if (ColMode < 0) ColMode = 0;
    if (ColMode >= NumColourModes) ColMode = NumColourModes - 1;
  }
  if (server.hasArg("speed")) {
    Speed = server.arg("speed").toInt();
    if (Speed < 0) Speed = 0;
    if (Speed > 255) Speed = 255;
  }
  if (server.hasArg("hue")) {
    Hue = server.arg("hue").toInt();
    if (Hue < 0) Hue = 0;
    if (Hue > 255) Hue = 255;
  }
  if (server.hasArg("saturation")) {
    Saturation = server.arg("saturation").toInt();
    if (Saturation < 0) Saturation = 0;
    if (Saturation > 255) Saturation = 255;
  }
  if (server.hasArg("brightness")) {
    Brightness = server.arg("brightness").toInt();
    if (Brightness < 0) Brightness = 0;
    if (Brightness > 255) Brightness = 255;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// -------- Pattern Functions -------- //

// Colour Mode 0: use fixed values from the webpage
void ColourMode0() {
  // No extra processing needed; parameters come from web inputs.
}

// Colour Mode 1: rainbow effect (automatically increment hue)
void ColourMode1() {
  Hue = (Hue + 1) % 255;
}

// Pattern Mode 0 - STATIC
void PatternMode0() {
  fill_solid(leds, currentLEDCount, CHSV(Hue, Saturation, Brightness));
}

// Pattern Mode 1 - SPARKLE
void PatternMode1() {
  int n = random(0, currentLEDCount);
  int x = (Hue + random(-10, 10)) % 255;
  if(x < 0) x += 255;
  int y = Saturation - random(0, 70);
  if(y < 0) y = 0;
  int z = Brightness - random(0, 80);
  if(z < 0) z = 0;
  leds[n] = CHSV(x, y, z);
  fadeToBlackBy(leds, currentLEDCount, 15);
}

// Pattern Mode 2 - COMETS
void PatternMode2() {
  for (int x = 0; x < NumComets; x++){
    if(ColMode == 0) {
      leds[CometLED[x]] = CHSV(Hue, Saturation, Brightness);
    } else if(ColMode == 1) {
      leds[CometLED[x]] = leds[CometLED[x]] + CHSV(CometColour[x], Saturation, Brightness);
    }
    
    if (CometCounter[x] < (LEDS_PER_STRIP - 1)) {
      CometLED[x] = CometLED[x] + CometDirection[x];
    } else {
      int RandPick = random(1, VertexConnections);
      for (int j = 0; j < VertexConnections; j++){
        for (int k = 0; k < NumVertices; k++){
          if (CometLED[x] == VertexArrayData[k][j]){
            CometStart[x] = VertexArrayData[k][((j + RandPick) % VertexConnections)];
          }
        }
      }
      CometLED[x] = CometStart[x];
      if ((CometLED[x] % LEDS_PER_STRIP) == 0){
        CometDirection[x] = 1;
      } else {
        CometDirection[x] = -1;
      }
    }
    CometCounter[x] = (CometCounter[x] + 1) % LEDS_PER_STRIP;
  }
  fadeToBlackBy(leds, currentLEDCount, 50);
}

// Pattern Mode 3 - FIREWORKS
void PatternMode3() {
  if (FireCounter < FireSize) {
    for (int x = 0; x < VertexConnections; x++){
      if (ColMode == 0) {
        leds[FireLED[x]] = CHSV(Hue, Saturation, Brightness);
      } else if (ColMode == 1) {
        leds[FireLED[x]] = CHSV(FireColour, Saturation, Brightness);
      }
      FireLED[x] = FireLED[x] + FireDirection[x];
    }
    FireCounter++;
    fadeToBlackBy(leds, currentLEDCount, 70);
  } else {
    FireCounter++;
    fadeToBlackBy(leds, currentLEDCount, 70);
    delay(50);
  }
  if (FireCounter > (FireSize + FireDelay)) {
    FireVertex = random(0, NumVertices);
    FireSize = random(3, LEDS_PER_STRIP);
    FireDelay = random(0, 20);
    FireColour = random8();
    for (int x = 0; x < VertexConnections; x++){
      FireStart[x] = VertexArrayData[FireVertex][x];
      FireLED[x] = VertexArrayData[FireVertex][x];
      if ((FireStart[x] % LEDS_PER_STRIP) == 0) {
        FireDirection[x] = 1;
      } else {
        FireDirection[x] = -1;
      }
    }
    FireCounter = 0;
    delay(50);
  }
}

// Pattern Mode 4 - MARQUEE
void PatternMode4() {
  if ((t % 30) == 0) {
    for (int i = 0; i < currentLEDCount; i++){
      if ((i % 2) == Marquee) {
        leds[i] = CHSV(Hue, Saturation, Brightness);
      } else {
        leds[i] = CRGB::Black;
      }
    }
    Marquee = (Marquee + 1) % 2;
  }
  t++;
}

// Helper functions for PACMAN (PatternMode5)
void ResetPacMan() {
  for (int i = 1; i < 5; i++){
    int RandPick = random(0, 3);
    PacManStart[i] = VertexArrayData[19][RandPick];
    PacManCounter[i] = 0;
    PacManLED[i] = PacManStart[i];
  }
  int RandPick = random(0, 3);
  PacManStart[0] = VertexArrayData[0][RandPick];
  PacManCounter[0] = 0;
  PacManLED[0] = PacManStart[0];
  for (int i = 0; i < 5; i++){
    if ((PacManStart[i] % LEDS_PER_STRIP) == 0) {
      PacManDirection[i] = 1;
    } else {
      PacManDirection[i] = -1;
    }
  }
}

void SetPacMan() {
  ResetPacMan();
  FastLED.clear();
  for (int i = 0; i < currentLEDCount; i++){
    if ((i % 2) == 0) {
      leds[i] = CHSV(10, 80, (Brightness / 10));
    } else {
      leds[i] = CRGB::Black;
    }
  }
  for (int i = 0; i < NumVertices; i++){
    for (int j = 0; j < VertexConnections; j++){
      leds[VertexArrayData[i][j]] = CHSV(165, 255, Brightness / 2);
    }
  }
}

void DiePacMan() {
  int Flash = 1;
  for (int i = 0; i < 5; i++){
    leds[PacManLED[i]] = CRGB::Black;
    FastLED.show();
  }
  ResetPacMan();
  for (int j = 0; j < 6; j++){
    for (int i = 0; i < NumVertices; i++){
      for (int k = 0; k < VertexConnections; k++){
        leds[VertexArrayData[i][k]] = CHSV(0, 255, Flash * (Brightness / 2));
      }
    }
    FastLED.show();
    delay(300);
    Flash = (Flash + 1) % 2;
  }
}

// Pattern Mode 5 - PACMAN
void PatternMode5() {
  if (Game == 0) {
    SetPacMan();
  }
  
  leds[PacManLED[0]] = CRGB::Black;
  PacManLED[0] = PacManLED[0] + PacManDirection[0];
  leds[PacManLED[0]] = CHSV(PacManColour[0], 255, Brightness);
  PacManCounter[0]++;
  
  if (PacManCounter[0] == (LEDS_PER_STRIP - 1)) {
    int RandPick = random(0, VertexConnections);
    for (int j = 0; j < VertexConnections; j++){
      for (int k = 0; k < NumVertices; k++){
        if (PacManLED[0] == VertexArrayData[k][j]){
          PacManStart[0] = VertexArrayData[k][((j + RandPick) % VertexConnections)];
        }
      }
    }
    PacManLED[0] = PacManStart[0];
    if ((PacManLED[0] % LEDS_PER_STRIP) == 0) {
      PacManDirection[0] = 1;
    } else {
      PacManDirection[0] = -1;
    }
    PacManCounter[0] = 0;
  }
  
  if ((Game % 2) == 0) {
    // GHOSTS
    for (int i = 1; i < 5; i++){
      leds[PacManLED[i]] = CRGB::Black;
      PacManLED[i] = PacManLED[i] + PacManDirection[i];
      leds[PacManLED[i]] = CHSV(PacManColour[i], 255, Brightness);
      PacManCounter[i]++;
      
      if (PacManLED[i] == PacManLED[0]) {
        DiePacMan();
      }
      
      if (PacManCounter[i] == (LEDS_PER_STRIP - 1)) {
        int RandPick = random(1, VertexConnections);
        for (int j = 0; j < VertexConnections; j++){
          for (int k = 0; k < NumVertices; k++){
            if (PacManLED[i] == VertexArrayData[k][j]){
              PacManStart[i] = VertexArrayData[k][((j + RandPick) % VertexConnections)];
            }
          }
        }
        PacManLED[i] = PacManStart[i];
        if ((PacManLED[i] % LEDS_PER_STRIP) == 0) {
          PacManDirection[i] = 1;
        } else {
          PacManDirection[i] = -1;
        }
        PacManCounter[i] = 0;
      }
    }
  }
  
  for (int i = 0; i < NumVertices; i++){
    for (int j = 0; j < VertexConnections; j++){
      leds[VertexArrayData[i][j]] = CHSV(165, 255, Brightness / 2);
    }
  }
  
  Game++;
  FastLED.show();
  delay(70);
}

// Pattern Mode 6 - CHRISTMAS
void PatternMode6() {
  int n = random(0, currentLEDCount);
  int x = random(0, 3);
  leds[n] = CHSV(ChristmasColour[0][x], ChristmasColour[1][x], Brightness);
  fadeToBlackBy(leds, currentLEDCount, 15);
}

// -------- Setup and Main Loop -------- //
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, MAX_LEDS);
  FastLED.clear();
  FastLED.show();
  
  // Initialize COMETS variables
  for (int i = 0; i < NumComets; i++){
    CometStart[i] = i * currentLEDCount / NumComets;
    CometCounter[i] = (i * currentLEDCount / NumComets) % LEDS_PER_STRIP;
    CometDirection[i] = 1;
    CometLED[i] = i * currentLEDCount / NumComets;
    CometColour[i] = (i * 270 / NumComets) % 255;
  }
  
  // Initialize FIREWORKS variables
  FireVertex = random(0, NumVertices);
  FireSize = random(3, LEDS_PER_STRIP);
  FireDelay = random(0, 20);
  for (int i = 0; i < VertexConnections; i++){
    FireStart[i] = VertexArrayData[FireVertex][i];
    FireLED[i] = VertexArrayData[FireVertex][i];
    if ((FireStart[i] % LEDS_PER_STRIP) == 0) {
      FireDirection[i] = 1;
    } else {
      FireDirection[i] = -1;
    }
  }
  
  // Connect to WiFi
  WiFi.begin("Perseverance", "AMeeZu5o?wifi");
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
  server.on("/setParams", HTTP_POST, handleSetParams);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  if (ledRunning) {
    // Process colour mode
    if (ColMode == 0)
      ColourMode0();
    else if (ColMode == 1)
      ColourMode1();
    
    // Execute selected pattern mode
    switch (PatMode) {
      case 0: PatternMode0(); break;
      case 1: PatternMode1(); break;
      case 2: PatternMode2(); break;
      case 3: PatternMode3(); break;
      case 4: PatternMode4(); break;
      case 5: PatternMode5(); break;
      case 6: PatternMode6(); break;
      default: PatternMode0(); break;
    }
    
    FastLED.show();
  } else {
    FastLED.clear();
    FastLED.show();
  }
}
