#include <Arduino_GFX_Library.h>



// --- DEFINICIONES DE COLORES ---
#define BLACK  0x0000
#define WHITE  0xFFFF
#define GREEN  0x07E0
#define BLUE   0x001F
#define RED    0xF800
#define YELLOW 0xFFE0

// --- CONFIGURACIÓN PANTALLA ST7789 ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(16, 5, 18, 23);
  Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 17, 3 /* Rotación */, true,
  170, 320, 35, 0, 35, 0);

// --- PINES DEL SISTEMA ---
const int sensorPin = 34;
const int speakerPin = 25;
const int redPin = 27; 
const int greenPin = 19;
const int bluePin = 4; 

// --- VARIABLES DE LÓGICA ---
int umbral = 2500; // Ajustado para mayor sensibilidad
int hz = 1000;
String mensajeMorse = "";
String palabraCompleta = "";
unsigned long tiempoInicio, tiempoApagado;
bool transmitiendo = false;
bool espacioAgregado = false;
bool letraProcesada = true;

// --- TRADUCTOR ---
String traducirMorse(String morse) {
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
  if (morse == ".--") return "W"; if (morse == "-..-") return "X";;
  if (morse == "-.--") return "Y"; if (morse == "--..") return "Z";
  if (morse == "-----") return "0";if (morse == ".----") return "1";
  if (morse == "..---") return "2";if (morse == "...--") return "3";
  if (morse == "....-") return "4";if (morse == ".....") return "5";
  if (morse == "-....") return "6";if (morse == "--...") return "7";
  if (morse == "---..") return "8";if (morse == "----.") return "9";
  return "?"; // Si no reconoce, no agrega nada
}

void actualizarPantalla() {
  // Encabezado fijo
  gfx->drawRect(0, 0, 320, 40, BLUE); 
  gfx->setCursor(40, 10); 
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println("COMUNICACION LI-FI");

  gfx->setCursor(15, 55);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->print("CODIGO:");
  
  // Área del código Morse (Puntos y rayas)
  gfx->fillRect(15, 80, 290, 35, BLACK); 
  gfx->setCursor(15, 85);
  gfx->setTextSize(2); 
  gfx->println(mensajeMorse);

  gfx->drawFastHLine(0, 120, 320, WHITE); 
  
  gfx->setCursor(15, 130);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->println("TRADUCCION:");
  
  // Área de la palabra traducida
  gfx->fillRect(15, 145, 300, 40, BLACK);
  gfx->setCursor(15, 145);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(3); 
  gfx->println(palabraCompleta);
}

void setup() {
  Serial.begin(115200);
  if (!gfx->begin()) {
    Serial.println("Error Pantalla");
    while(1);
  }
  
  gfx->fillScreen(BLACK);
  pinMode(sensorPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  // Apagar LED azul que causaba confusión
  digitalWrite(bluePin, LOW);
  
  actualizarPantalla();
}

void loop() {
  int valorLuz = analogRead(sensorPin);

  // --- DETECCIÓN DE PULSOS ---
  if (valorLuz > umbral) {
    if (!transmitiendo) {
      tiempoInicio = millis();
      transmitiendo = true;
      letraProcesada = false;
      espacioAgregado = false; // Resetear bandera de espacio al recibir señal
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
      
      // Clasificación de pulso
      if (duracion > 20 && duracion < 300) mensajeMorse += ".";
      else if (duracion >= 300) mensajeMorse += "-";
      
      actualizarPantalla();
    }
  }

  // --- PROCESAR LETRA (Final de una letra: pausa de 450ms) ---
  if (!transmitiendo && !letraProcesada && (millis() - tiempoApagado > 450) && mensajeMorse != "") {
    String letra = traducirMorse(mensajeMorse);
    palabraCompleta += letra;
    mensajeMorse = ""; 
    letraProcesada = true;
    actualizarPantalla();
  }

  // --- ESPACIO ENTRE PALABRAS (Pausa de 1 segundos) ---
  if (!transmitiendo && (millis() - tiempoApagado > 1000) && !espacioAgregado && palabraCompleta != "") {
    if (!palabraCompleta.endsWith(" ")) {
      palabraCompleta += " ";
    }
    espacioAgregado = true; 
    actualizarPantalla();
  }
  // --- LIMPIEZA AUTOMÁTICA POR INACTIVIDAD (5 segundos) ---
  if (!transmitiendo && (millis() - tiempoApagado > 5000) && palabraCompleta != "") {
    palabraCompleta = "";
    mensajeMorse = "";
    letraProcesada = true;
    espacioAgregado = true;
    actualizarPantalla();
  }
}
