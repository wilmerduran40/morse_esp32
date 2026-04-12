#include <Arduino_GFX_Library.h>

// --- DEFINICIONES DE COLORES ---
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BLUE  0x001F
#define RED   0xF800
#define YELLOW 0xFFE0

// --- CONFIGURACIÓN PANTALLA ST7789 ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(16, 5, 18, 23);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 17, 3 /* Rotación Horizontal */, true,
  170, 320, 35, 0, 35, 0);

// --- PINES DEL SISTEMA ---
const int sensorPin = 34;
const int speakerPin = 25;
const int redPin = 27; 
const int greenPin = 19;
const int bluePin = 4; 

// --- VARIABLES DE LÓGICA ---
int umbral = 3500;
int hz = 1000;
String mensajeMorse = "";
String palabraCompleta = "";
unsigned long tiempoInicio, tiempoApagado;
bool transmitiendo = false;
bool espacioAgregado = false;

void actualizarPantalla() {
  gfx->fillScreen(BLACK);
  gfx->drawRect(0, 0, 320, 40, BLUE); 
  gfx->setCursor(40, 10); 
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println("COMUNICACION LI-FI");

  gfx->setCursor(15, 55);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->print("CODIGO:");
  
  gfx->setCursor(15, 85);
  gfx->setTextSize(4); 
  gfx->println(mensajeMorse);

  gfx->drawFastHLine(0, 120, 320, WHITE); 
  
  gfx->setCursor(15, 130);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->println("TRADUCCION:");
  
  gfx->setCursor(15, 145);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(3); 
  gfx->println(palabraCompleta);
}

String traducirMorse(String morse) {
  if (morse == ".....") return "RESET"; 
  if (morse == "...---...") return "SOS";
  if (morse == ".-") return "A"; if (morse == "-...") return "B";
  if (morse == "-.-.") return "C"; if (morse == "-..") return "D";
  if (morse == ".") return "E"; if (morse == "..-.") return "F";
  if (morse == "--.") return "G"; if (morse == "....") return "H";
  if (morse == "..") return "I"; if (morse == ".---") return "J";
  if (morse == "-.-") return "K"; if (morse == ".-..") return "L";
  if (morse == "--") return "M"; if (morse == "-.") return "N";
  if (morse == "---") return "O"; if (morse == ".--.") return "P";
  if (morse == "--.-") return "Q"; if (morse == ".-.") return "R";
  if (morse == "...") return "S"; if (morse == "-") return "T";
  if (morse == "..-") return "U"; if (morse == "...-") return "V";
  if (morse == ".--") return "W"; if (morse == "-..-") return "X";
  if (morse == "-.--") return "Y"; if (morse == "--..") return "Z";
  return "?";
}

void setup() {
  Serial.begin(115200);
  if (!gfx->begin()) { Serial.println("Error Pantalla"); }
  actualizarPantalla();

  pinMode(sensorPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void loop() {
  int valorLuz = analogRead(sensorPin);

  // --- RESET POR BOTÓN FÍSICO (PIN 34) ---
  if (valorLuz > 4000 && palabraCompleta != "") {
    palabraCompleta = "";
    mensajeMorse = "";
    actualizarPantalla();
    tone(speakerPin, 1500, 200);
    digitalWrite(bluePin, HIGH);
    delay(500); // Evitar rebotes
  }

  // Sonido y LED Verde en tiempo real
  if (valorLuz > umbral) {
    tone(speakerPin, hz);
    digitalWrite(greenPin, HIGH);
    digitalWrite(bluePin, LOW);
  } else {
    noTone(speakerPin);
    digitalWrite(greenPin, LOW);
  }

  // Detectar inicio de pulso
  if (valorLuz > umbral && !transmitiendo) {
    tiempoInicio = millis();
    transmitiendo = true;
    espacioAgregado = false;
  }

  // Detectar fin de pulso
  if (valorLuz < (umbral - 200) && transmitiendo) {
    unsigned long duracion = millis() - tiempoInicio;
    transmitiendo = false;
    tiempoApagado = millis();
    if (duracion > 30 && duracion < 350) mensajeMorse += ".";
    else if (duracion >= 350) mensajeMorse += "-";
    actualizarPantalla();
  }

  // PROCESAR LETRA O COMANDO RESET
  if (!transmitiendo && (millis() - tiempoApagado > 400) && mensajeMorse != "") {
    String resultado = traducirMorse(mensajeMorse);
    
    if (resultado == "RESET") {
      palabraCompleta = "";
      mensajeMorse = "";
      tone(speakerPin, 1500, 500);
      digitalWrite(bluePin, HIGH);
    } else {
      palabraCompleta += resultado;
      mensajeMorse = "";
      digitalWrite(redPin, HIGH); delay(50); digitalWrite(redPin, LOW);
    }
    actualizarPantalla();
  }

  // ESPACIO (0.7 seg)
  if (!transmitiendo && (millis() - tiempoApagado > 700) && !espacioAgregado && palabraCompleta != "") {
    palabraCompleta += " ";
    espacioAgregado = true; 
    actualizarPantalla();
  }

  // REINICIAR AUTOMÁTICO (5 seg)
  if (!transmitiendo && (millis() - tiempoApagado > 5000) && palabraCompleta != "") {
    palabraCompleta = "";
    actualizarPantalla();
  }
}
