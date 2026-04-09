#include <Arduino_GFX_Library.h>

// --- DEFINICIONES DE COLORES ---
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BLUE  0x001F
#define RED   0xF800
#define YELLOW 0xFFE0

// --- CONFIGURACIÓN PANTALLA ST7789 ---
// Pines: DC=16, CS=5, SCK=18, MOSI=23, RST=17
Arduino_DataBus *bus = new Arduino_ESP32SPI(16, 5, 18, 23);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 17, 3 /* Rotación hacia abajo */, true,
  170, 320, 35, 0, 35, 0);

// --- PINES DEL SISTEMA ---
const int sensorPin = 34;
const int speakerPin = 25;
const int redPin = 27;   // Cambiado para no chocar con SPI (era 18)
const int greenPin = 19;
const int bluePin = 4;   // Cambiado para no chocar con SPI (era 17)
  // --- RESET MANUAL POR BOTÓN EN PIN 34 ---
  int lecturaBoton = analogRead(sensorPin); 

// --- VARIABLES DE LÓGICA ---
int umbral = 3500;
int hz = 1000;
String mensajeMorse = "";
String palabraCompleta = "";
unsigned long tiempoInicio, tiempoApagado;
bool transmitiendo = false;
bool espacioAgregado = false;

// --- FUNCIÓN PARA DIBUJAR EN LA TFT ---
void actualizarPantalla() {
  gfx->fillScreen(BLACK);
  
  // 1. Título con marco azul
  gfx->drawRect(0, 0, 320, 40, BLUE); 
  gfx->setCursor(40, 10); 
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println("COMUNICACION LI-FI");

  // 2. Sección Morse (Puntos y rayas)
  gfx->setCursor(15, 55);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->print("CODIGO:");
  
  gfx->setCursor(15, 85);
  gfx->setTextSize(4); // Morse más grande (Tamaño 4) para que resalte
  gfx->println(mensajeMorse);

  // 3. Línea divisoria
  gfx->drawFastHLine(0, 120, 320, WHITE); 
  
  // 4. Traducción final
  gfx->setCursor(15, 130);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1); // Etiqueta pequeña
  gfx->println("TRADUCCION:");
  
  gfx->setCursor(15, 145);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(3); // Resultado en verde y grande
  gfx->println(palabraCompleta);
}

String traducirMorse(String morse) {
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
  if (morse == "...---...") return  "SOS";
  return "?";
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar Pantalla
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

  // FIN DE LETRA (0.35 seg)
  if (!transmitiendo && (millis() - tiempoApagado > 350) && mensajeMorse != "") {
    palabraCompleta += traducirMorse(mensajeMorse);
    mensajeMorse = "";
    digitalWrite(redPin, HIGH); delay(50); digitalWrite(redPin, LOW);
    actualizarPantalla();
  }

  // DETECTAR ESPACIO (0.65 seg)
  if (!transmitiendo && (millis() - tiempoApagado > 700) && !espacioAgregado && palabraCompleta != "") {
    palabraCompleta += " ";
    espacioAgregado = true; 
    actualizarPantalla();
  }

  // REINICIAR (5 seg)
  if (!transmitiendo && (millis() - tiempoApagado > 10000) && palabraCompleta != "") {
    palabraCompleta = "";
    actualizarPantalla();

  }


    

    
    
  
}
