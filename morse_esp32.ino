#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pines
const int sensorPin = 34;
const int speakerPin = 25;
const int redPin = 18;
const int greenPin = 19;
const int bluePin = 17; 

// Variables de lógica
int umbral = 2500;
int hz= 1000;
String mensajeMorse = "";
String palabraCompleta = "";
unsigned long tiempoInicio, tiempoApagado;
bool transmitiendo = false;
bool espacioAgregado = false; // Control para no repetir espacios

void actualizarOLED() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Título
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("RECEPTOR LI-FI");
  display.drawLine(0, 10, 128, 10, WHITE);
  
  // Morse actual (Lo que se ve en el momento de enviar codigo morse )
  display.setCursor(0, 15);
  display.setTextSize(2);
  display.print("-> "); 
  display.println(mensajeMorse);
  
  // Mensaje acumulado
  display.setCursor(0, 45);
  display.setTextSize(1);
  display.print("MSG: ");
  display.println(palabraCompleta);
  
  display.display();
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
  return "?";
}

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;);  // este for se usa por seguridad si la pantalla no responde se queda esperando aqui
  }
  actualizarOLED();

  pinMode(sensorPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  digitalWrite(bluePin, HIGH);
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
    espacioAgregado = false; // Permitir nuevo espacio tras detectar luz
  }

  // Detectar fin de pulso (punto o raya)
  if (valorLuz < (umbral - 200) && transmitiendo) {
    unsigned long duracion = millis() - tiempoInicio;
    transmitiendo = false;
    tiempoApagado = millis();

    if (duracion > 30 && duracion < 350) mensajeMorse += ".";
    else if (duracion >= 350) mensajeMorse += "-";
    
    actualizarOLED();
  }

  // --- LÓGICA DE TIEMPOS DE ESPERA ---

  // 1. FIN DE LETRA (0.35 segundos de silencio)
  if (!transmitiendo && (millis() - tiempoApagado > 350) && mensajeMorse != "") {
    palabraCompleta += traducirMorse(mensajeMorse);
    mensajeMorse = "";
    digitalWrite(redPin, HIGH); delay(50); digitalWrite(redPin, LOW);
    actualizarOLED();
  }

  // 2. DETECTAR ESPACIO (0.65 segundos de silencio)
  if (!transmitiendo && (millis() - tiempoApagado > 650) && !espacioAgregado && palabraCompleta != "") {
    palabraCompleta += " "; // Agregamos el espacio
    espacioAgregado = true; 
    actualizarOLED();
  }

  // 3. REINICIAR PANTALLA (10 segundos de inactividad)
  if (!transmitiendo && (millis() - tiempoApagado > 10000) && palabraCompleta != "") {
    palabraCompleta = "";
    actualizarOLED();
    digitalWrite(bluePin, HIGH);
  }
}