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
#define MAX_STRIPS     30               // Always 30 strips
#define MAX_LEDS       (LEDS_PER_STRIP * MAX_STRIPS)

CRGB leds[MAX_LEDS];
const int currentLEDCount = MAX_LEDS;  // Always drive all LEDs

// -------- Global pattern parameters -------- //
bool ledRunning = true;                // LED effect on/off
int Speed = 50;         // (0-255) — affects some patterns
int Hue = 0;            // (0-255) — used in monochromatic mode; in rainbow mode auto–incremented
int Saturation = 255;   // (0-255)
int Brightness = 128;   // (0-255)

// Color mode: 0 = Monochromatic, 1 = Rainbow
int ColorMode = 0;

// Pattern mode (0 to 14)
int PatMode = 0;
const int NumPatternModes = 15;

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
int VertexArrayData[NumVertices][VertexConnections] = {
  {84,  85,   0},  // A
  {16,  17, 152},  // B
  {101,102,339},   // C
  {67,  68, 305},  // D
  {50,  51, 254},  // E
  {33,  34, 203},  // F
  {135,136,153},   // G
  {118,119,390},   // H
  {322,323,340},   // I
  {288,289,306},   // J
  {271,272,492},   // K
  {237,238,255},   // L
  {186,187,204},   // M
  {169,170,424},   // N
  {373,374,391},   // O
  {356,357,509},   // P
  {475,476,493},   // Q
  {220,221,458},   // R
  {407,408,425},   // S
  {441,442,459}    // T
};

// -------- Web server -------- //
WebServer server(80);

// -------- HTML page -------- //
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Infinity dodecahedron</title>";
  // Dark mode CSS styling:
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: #121212; color: #e0e0e0; margin: 20px; }";
  html += "h1, h2 { color: #ffffff; }";
  html += "form { margin: 10px 0; padding: 10px; background: #1e1e1e; border-radius: 5px; }";
  html += ".slider { width: 100%; }";
  html += ".button { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; background: #4285f4; color: white; cursor: pointer; }";
  html += ".button:hover { background: #357ae8; }";
  html += "</style></head><body>";
  
  html += "<h1>Infinity dodecahedron</h1>";
  
  // On/Off toggle button (text reflects current state)
  html += "<form action='/toggle' method='GET'>";
  html += "<button class='button' type='submit'>" + String(ledRunning ? "On" : "Off") + "</button>";
  html += "</form>";
  
  // Sliders and controls container
  html += "<h2>Set Parameters</h2>";
  
  html += "<div id='div_speed'>Speed: <input id='speed' class='slider' type='range' name='speed' min='0' max='255' value='" + String(Speed) + "' oninput='updateParam(\"speed\", this.value); document.getElementById(\"speedOutput\").value = this.value;'> <output id='speedOutput'>" + String(Speed) + "</output></div>";
  html += "<div id='div_hue'>Hue: <input id='hue' class='slider' type='range' name='hue' min='0' max='255' value='" + String(Hue) + "' oninput='updateParam(\"hue\", this.value); document.getElementById(\"hueOutput\").value = this.value;'> <output id='hueOutput'>" + String(Hue) + "</output></div>";
  html += "<div id='div_saturation'>Saturation: <input id='saturation' class='slider' type='range' name='saturation' min='0' max='255' value='" + String(Saturation) + "' oninput='updateParam(\"saturation\", this.value); document.getElementById(\"saturationOutput\").value = this.value;'> <output id='saturationOutput'>" + String(Saturation) + "</output></div>";
  html += "<div id='div_brightness'>Brightness: <input id='brightness' class='slider' type='range' name='brightness' min='0' max='255' value='" + String(Brightness) + "' oninput='updateParam(\"brightness\", this.value); document.getElementById(\"brightnessOutput\").value = this.value;'> <output id='brightnessOutput'>" + String(Brightness) + "</output></div>";
  // Color Mode control; only shown when applicable:
  html += "<div id='div_colormode'>Color Mode: ";
  html += "<input type='radio' name='colormode' value='0' onchange='updateColorMode(0)' id='mono' " + String((ColorMode==0)?"checked":"") + ">";
  html += "<label for='mono'>Monochromatic</label> ";
  html += "<input type='radio' name='colormode' value='1' onchange='updateColorMode(1)' id='rainbow' " + String((ColorMode==1)?"checked":"") + ">";
  html += "<label for='rainbow'>Rainbow</label></div>";
  
  // Pattern selection buttons
  html += "<h2>Select Pattern</h2>";
  html += "<form action='/setPattern' method='GET'>";
  html += "<button class='button' type='submit' name='mode' value='0'>Static</button>";
  html += "<button class='button' type='submit' name='mode' value='1'>Sparkle</button>";
  html += "<button class='button' type='submit' name='mode' value='2'>Comets</button>";
  html += "<button class='button' type='submit' name='mode' value='3'>Fireworks</button>";
  html += "<button class='button' type='submit' name='mode' value='4'>Marquee</button>";
  html += "<button class='button' type='submit' name='mode' value='5'>PacMan</button>";
  html += "<button class='button' type='submit' name='mode' value='6'>Christmas</button><br>";
  html += "<button class='button' type='submit' name='mode' value='7'>Breathing</button>";
  html += "<button class='button' type='submit' name='mode' value='8'>Spiral</button>";
  html += "<button class='button' type='submit' name='mode' value='9'>Ripple</button>";
  html += "<button class='button' type='submit' name='mode' value='10'>Kaleidoscope</button>";
  html += "<button class='button' type='submit' name='mode' value='11'>Particle</button>";
  html += "<button class='button' type='submit' name='mode' value='12'>Matrix Rain</button>";
  html += "<button class='button' type='submit' name='mode' value='13'>Aurora</button>";
  html += "<button class='button' type='submit' name='mode' value='14'>Glitter</button>";
  html += "</form>";
  
  // JavaScript for immediate updates and slider visibility:
  html += "<script>";
  html += "function updateParam(param, value) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/setParams?' + param + '=' + value, true);";
  html += "  xhr.send();";
  html += "}";
  html += "function updateColorMode(mode) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/setColorMode?mode=' + mode, true);";
  html += "  xhr.send();";
  html += "}";
  // Set currentPattern from server-side variable PatMode:
  html += "var currentPattern = " + String(PatMode) + ";";
  html += "var sliderVisibility = {";
  html += "  0: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  1: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  2: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  3: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  4: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  5: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  6: {speed: false, hue: false, saturation: false, brightness: true, colormode: false},";
  html += "  7: {speed: true, hue: true, saturation: true, brightness: true, colormode: true},";
  html += "  8: {speed: true, hue: false, saturation: true, brightness: true, colormode: false},";
  html += "  9: {speed: true, hue: true, saturation: true, brightness: true, colormode: true},";
  html += " 10: {speed: false, hue: true, saturation: true, brightness: true, colormode: false},";
  html += " 11: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += " 12: {speed: false, hue: true, saturation: true, brightness: true, colormode: true},";
  html += " 13: {speed: true, hue: true, saturation: true, brightness: true, colormode: true},";
  html += " 14: {speed: false, hue: true, saturation: true, brightness: true, colormode: true}";
  html += "};";
  html += "function updateSliderVisibility() {";
  html += "  document.getElementById('div_speed').style.display = sliderVisibility[currentPattern].speed ? 'block' : 'none';";
  html += "  document.getElementById('div_hue').style.display = sliderVisibility[currentPattern].hue ? 'block' : 'none';";
  html += "  document.getElementById('div_saturation').style.display = sliderVisibility[currentPattern].saturation ? 'block' : 'none';";
  html += "  document.getElementById('div_brightness').style.display = sliderVisibility[currentPattern].brightness ? 'block' : 'none';";
  html += "  document.getElementById('div_colormode').style.display = sliderVisibility[currentPattern].colormode ? 'block' : 'none';";
  html += "}";
  html += "window.onload = updateSliderVisibility;";
  html += "</script>";
  
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

void handleSetParams() {
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
  server.send(200, "text/plain", "OK");
}

void handleSetColorMode() {
  if (server.hasArg("mode")) {
    ColorMode = server.arg("mode").toInt();
    if (ColorMode < 0) ColorMode = 0;
    if (ColorMode > 1) ColorMode = 1;
  }
  server.send(200, "text/plain", "OK");
}

void handleSetPattern() {
  if (server.hasArg("mode")) {
    PatMode = server.arg("mode").toInt();
    if (PatMode < 0) PatMode = 0;
    if (PatMode >= NumPatternModes) PatMode = NumPatternModes - 1;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// -------- Pattern Functions -------- //

// In Rainbow mode, update hue automatically; in Monochromatic mode, hue remains as set.
void updateHue() {
  if (ColorMode == 1) {
    Hue = (Hue + 1) % 255;
  }
}

// Pattern Mode 0 - STATIC
void PatternMode0() {
  fill_solid(leds, currentLEDCount, CHSV(Hue, Saturation, Brightness));
}

// Pattern Mode 1 - SPARKLE
void PatternMode1() {
  int n = random(0, currentLEDCount);
  int x = (Hue + random(-10, 10)) % 255;
  if (x < 0) x += 255;
  int y = Saturation - random(0, 70);
  if (y < 0) y = 0;
  int z = Brightness - random(0, 80);
  if (z < 0) z = 0;
  leds[n] = CHSV(x, y, z);
  fadeToBlackBy(leds, currentLEDCount, 15);
}

// Pattern Mode 2 - COMETS
void PatternMode2() {
  for (int x = 0; x < NumComets; x++) {
    leds[CometLED[x]] = leds[CometLED[x]] + CHSV(CometColour[x], Saturation, Brightness);
    
    if (CometCounter[x] < (LEDS_PER_STRIP - 1)) {
      CometLED[x] = CometLED[x] + CometDirection[x];
    } else {
      int RandPick = random(1, VertexConnections);
      for (int j = 0; j < VertexConnections; j++) {
        for (int k = 0; k < NumVertices; k++) {
          if (CometLED[x] == VertexArrayData[k][j]) {
            CometStart[x] = VertexArrayData[k][((j + RandPick) % VertexConnections)];
          }
        }
      }
      CometLED[x] = CometStart[x];
      if ((CometLED[x] % LEDS_PER_STRIP) == 0) {
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
    for (int x = 0; x < VertexConnections; x++) {
      leds[FireLED[x]] = CHSV(FireColour, Saturation, Brightness);
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
    for (int x = 0; x < VertexConnections; x++) {
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
    for (int i = 0; i < currentLEDCount; i++) {
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

// Pattern Mode 5 - PACMAN
void PatternMode5() {
  if (Game == 0) {
    for (int i = 1; i < 5; i++) {
      int RandPick = random(0, 3);
      PacManStart[i] = VertexArrayData[19][RandPick];
      PacManCounter[i] = 0;
      PacManLED[i] = PacManStart[i];
    }
    int RandPick = random(0, 3);
    PacManStart[0] = VertexArrayData[0][RandPick];
    PacManCounter[0] = 0;
    PacManLED[0] = PacManStart[0];
    for (int i = 0; i < 5; i++) {
      if ((PacManStart[i] % LEDS_PER_STRIP) == 0) {
        PacManDirection[i] = 1;
      } else {
        PacManDirection[i] = -1;
      }
    }
  }
  
  leds[PacManLED[0]] = CRGB::Black;
  PacManLED[0] = PacManLED[0] + PacManDirection[0];
  leds[PacManLED[0]] = CHSV(PacManColour[0], 255, Brightness);
  PacManCounter[0]++;
  
  if (PacManCounter[0] == (LEDS_PER_STRIP - 1)) {
    int RandPick = random(0, VertexConnections);
    for (int j = 0; j < VertexConnections; j++) {
      for (int k = 0; k < NumVertices; k++) {
        if (PacManLED[0] == VertexArrayData[k][j]) {
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
    for (int i = 1; i < 5; i++) {
      leds[PacManLED[i]] = CRGB::Black;
      PacManLED[i] = PacManLED[i] + PacManDirection[i];
      leds[PacManLED[i]] = CHSV(PacManColour[i], 255, Brightness);
      PacManCounter[i]++;
      
      if (PacManLED[i] == PacManLED[0]) {
        Game = 0;
      }
      
      if (PacManCounter[i] == (LEDS_PER_STRIP - 1)) {
        int RandPick = random(1, VertexConnections);
        for (int j = 0; j < VertexConnections; j++) {
          for (int k = 0; k < NumVertices; k++) {
            if (PacManLED[i] == VertexArrayData[k][j]) {
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
  
  for (int i = 0; i < NumVertices; i++) {
    for (int j = 0; j < VertexConnections; j++) {
      leds[VertexArrayData[i][j]] = CHSV(165, 255, Brightness / 2);
    }
  }
  
  Game++;
  delay(70);
}

// Pattern Mode 6 - CHRISTMAS
void PatternMode6() {
  int n = random(0, currentLEDCount);
  int x = random(0, 3);
  leds[n] = CHSV(ChristmasColour[0][x], ChristmasColour[1][x], Brightness);
  fadeToBlackBy(leds, currentLEDCount, 15);
}

// --------------- New Patterns ----------------- //

// Pattern Mode 7 - Breathing (Pulsing) Effect (slightly slower, smooth color)
void PatternMode7() {
  static uint32_t lastHueTime = 0;
  static uint8_t slowHue = Hue;
  if (millis() - lastHueTime > 1500) { // update hue every 1.5 seconds
    slowHue = (slowHue + 1) % 255;
    lastHueTime = millis();
  }
  static uint16_t breathCounter = 0;
  breathCounter++;
  uint8_t breath = sin8(breathCounter >> 3); // slower breathing curve
  fill_solid(leds, currentLEDCount, CHSV(slowHue, Saturation, (Brightness * breath) / 255));
}

// Pattern Mode 8 - Spiral/Rotational Effect (faster)
void PatternMode8() {
  static int offset = 0;
  offset = (offset + Speed/10 + 1) % 255;
  for (int i = 0; i < currentLEDCount; i++) {
    uint8_t pos = (i + offset) % 255;
    leds[i] = CHSV(pos, Saturation, Brightness);
  }
}

// Pattern Mode 9 - Ripple/Wave Effect
void PatternMode9() {
  static uint32_t timeCounter = 0;
  timeCounter++;
  for (int i = 0; i < currentLEDCount; i++) {
    uint8_t wave = sin8(i * 8 + timeCounter * (Speed/10 + 1));
    leds[i] = CHSV(Hue, Saturation, (Brightness * wave) / 255);
  }
}

// Pattern Mode 10 - Kaleidoscopic Symmetry
void PatternMode10() {
  int half = currentLEDCount / 2;
  uint32_t t = millis() * Speed / 100;
  for (int i = 0; i < half; i++) {
    uint8_t pos = (i + t) % 255;
    CRGB col = CHSV(pos, Saturation, Brightness);
    leds[i] = col;
    leds[currentLEDCount - 1 - i] = col;
  }
  if (currentLEDCount % 2 == 1) {
    leds[half] = CHSV(Hue, Saturation, Brightness);
  }
}

// Pattern Mode 11 - Particle/Firefly Simulation
#define NUM_PARTICLES 10
void PatternMode11() {
  static int particlePositions[NUM_PARTICLES];
  static uint8_t particleBrightness[NUM_PARTICLES];
  static bool init = false;
  if (!init) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
      particlePositions[i] = random(0, currentLEDCount);
      particleBrightness[i] = 0;
    }
    init = true;
  }
  fadeToBlackBy(leds, currentLEDCount, 20);
  int idx = random(0, NUM_PARTICLES);
  particlePositions[idx] = random(0, currentLEDCount);
  particleBrightness[idx] = Brightness;
  
  for (int i = 0; i < NUM_PARTICLES; i++) {
    if (particleBrightness[i] > 0) {
      leds[particlePositions[i]] = CHSV(Hue, Saturation, particleBrightness[i]);
      particleBrightness[i] = (particleBrightness[i] > 5 ? particleBrightness[i] - 5 : 0);
    }
  }
}

// Pattern Mode 12 - Digital Rain / Matrix Effect
void PatternMode12() {
  static int dropPositions[MAX_STRIPS];
  static bool init = false;
  if (!init) {
    for (int s = 0; s < MAX_STRIPS; s++) {
      dropPositions[s] = random(0, LEDS_PER_STRIP);
    }
    init = true;
  }
  fadeToBlackBy(leds, currentLEDCount, 50);
  for (int s = 0; s < MAX_STRIPS; s++) {
    int baseIndex = s * LEDS_PER_STRIP;
    leds[baseIndex + dropPositions[s]] = CHSV(Hue, Saturation, Brightness);
    dropPositions[s] = (dropPositions[s] + 1) % LEDS_PER_STRIP;
  }
}

// Pattern Mode 13 - Aurora Borealis Style
void PatternMode13() {
  static uint32_t timeCounter = 0;
  timeCounter++;
  for (int i = 0; i < currentLEDCount; i++) {
    uint8_t localHue = (Hue + sin8(i * 16 + timeCounter * (Speed/10 + 1))) % 255;
    uint8_t localBright = (Brightness * sin8(i * 8 + timeCounter * (Speed/15 + 1))) / 255;
    leds[i] = CHSV(localHue, Saturation, localBright);
  }
}

// Pattern Mode 14 - Glitter/Starfield
void PatternMode14() {
  fadeToBlackBy(leds, currentLEDCount, 10);
  int sparkles = currentLEDCount / 50;
  for (int i = 0; i < sparkles; i++) {
    int pos = random(0, currentLEDCount);
    leds[pos] = CHSV(Hue, Saturation, Brightness);
  }
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
  for (int i = 0; i < NumComets; i++) {
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
  for (int i = 0; i < VertexConnections; i++) {
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
  ArduinoOTA.onStart([]() { Serial.println("Start OTA update"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd OTA update"); });
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
  server.on("/setParams", HTTP_GET, handleSetParams);
  server.on("/setColorMode", HTTP_GET, handleSetColorMode);
  server.on("/setPattern", HTTP_GET, handleSetPattern);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  if (ledRunning) {
    // Update hue if in rainbow mode (except for breathing, which uses its own slow update)
    if (PatMode != 7) updateHue();
    
    // Execute selected pattern mode
    switch (PatMode) {
      case 0: PatternMode0(); break;
      case 1: PatternMode1(); break;
      case 2: PatternMode2(); break;
      case 3: PatternMode3(); break;
      case 4: PatternMode4(); break;
      case 5: PatternMode5(); break;
      case 6: PatternMode6(); break;
      case 7: PatternMode7(); break;
      case 8: PatternMode8(); break;
      case 9: PatternMode9(); break;
      case 10: PatternMode10(); break;
      case 11: PatternMode11(); break;
      case 12: PatternMode12(); break;
      case 13: PatternMode13(); break;
      case 14: PatternMode14(); break;
      default: PatternMode0(); break;
    }
    
    FastLED.show();
  } else {
    FastLED.clear();
    FastLED.show();
  }
}
