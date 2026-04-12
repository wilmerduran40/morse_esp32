#include <Arduino_GFX_Library.h>

// --- DEFINICIONES DE COLORES ---
#define BLACK  0x0000
#define WHITE  0xFFFF
#define GREEN  0x07E0
#define BLUE   0x001F
#define RED    0xF800
#define YELLOW 0xFFE0

// --- CONFIGURACIÓN PANTALLA ST7789 ---
// Ajusta los pines según tu conexión física si es necesario
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
bool letraProcesada = true;

// --- FUNCIONES ---

String traducirMorse(String morse) {
  if (morse == ".....") return " "; // 5 puntos para espacio manual
  if (morse == "---") return "O";   // Ejemplo específico
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

void actualizarPantalla() {
  // Dibujar encabezado solo si es necesario (puedes mover esto al setup)
  gfx->drawRect(0, 0, 320, 40, BLUE); 
  gfx->setCursor(40, 10); 
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println("COMUNICACION LI-FI");

  gfx->setCursor(15, 55);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->print("CODIGO:");
  
  // Limpiar y escribir el código Morse actual
  gfx->fillRect(15, 80, 290, 35, BLACK); 
  gfx->setCursor(15, 85);
  gfx->setTextSize(4); 
  gfx->println(mensajeMorse);

  gfx->drawFastHLine(0, 120, 320, WHITE); 
  
  gfx->setCursor(15, 130);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->println("TRADUCCION:");
  
  // Limpiar y escribir la palabra completa
  gfx->fillRect(15, 145, 300, 40, BLACK);
  gfx->setCursor(15, 145);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(3); 
  gfx->println(palabraCompleta);
}

void setup() {
  Serial.begin(115200);
  if (!gfx->begin()) { Serial.println("Error Pantalla"); }
  
  gfx->fillScreen(BLACK);
  pinMode(sensorPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  actualizarPantalla();
}

void loop() {
  int valorLuz = analogRead(sensorPin);

  // --- REINICIO MANUAL (Por intensidad de luz muy alta) ---
  if (valorLuz > 4050) {
    palabraCompleta = "";
    mensajeMorse = "";
    gfx->fillScreen(BLACK);
    actualizarPantalla();
    tone(speakerPin, 1500, 300);
    digitalWrite(bluePin, HIGH);
    delay(1000);
    digitalWrite(bluePin, LOW);
    return;
  }

  // --- DETECCIÓN DE PULSOS (LÓGICA DE TIEMPO REAL) ---
  if (valorLuz > umbral) {
    if (!transmitiendo) {
      tiempoInicio = millis();
      transmitiendo = true;
      letraProcesada = false;
    }
    tone(speakerPin, hz);
    digitalWrite(greenPin, HIGH);
  } else {
    noTone(speakerPin);
    digitalWrite(greenPin, LOW);
    
    if (transmitiendo) {
      unsigned long duracion = millis() - tiempoInicio;
      transmitiendo = false;
      tiempoApagado = millis();
      
      if (duracion > 20 && duracion < 300) mensajeMorse += ".";
      else if (duracion >= 300) mensajeMorse += "-";
      
      actualizarPantalla();
    }
  }

  // --- PROCESAR LETRA (Si pasan más de 600ms sin pulso) ---
  if (!transmitiendo && !letraProcesada && (millis() - tiempoApagado > 600) && mensajeMorse != "") {
    String letra = traducirMorse(mensajeMorse);
    if (letra != "?") {
      palabraCompleta += letra;
    }
    mensajeMorse = ""; // Limpiamos el buffer para la siguiente letra
    letraProcesada = true;
    actualizarPantalla();
  }

  // --- AÑADIR ESPACIO AUTOMÁTICO (Si pasan más de 2 segundos) ---
  if (!transmitiendo && (millis() - tiempoApagado > 2000) && !espacioAgregado && palabraCompleta != "") {
    if (!palabraCompleta.endsWith(" ")) {
      palabraCompleta += " ";
    }
    espacioAgregado = true; 
    actualizarPantalla();
  }

  // --- LIMPIEZA AUTOMÁTICA POR INACTIVIDAD (10 segundos) ---
  if (!transmitiendo && (millis() - tiempoApagado > 10000) && palabraCompleta != "") {
    palabraCompleta = "";
    mensajeMorse = "";
    actualizarPantalla();
  }
}
