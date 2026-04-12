#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURACIÓN DE RED ---
const char* ssid = "WIFI";
const char* password = "Duran1612.";

// --- DEFINICIONES DE COLORES ---
#define BLACK  0x0000
#define WHITE  0xFFFF
#define GREEN  0x07E0
#define BLUE   0x001F
#define RED    0xF800
#define YELLOW 0xFFE0

// --- CONFIGURACIÓN PANTALLA ST7789 ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(16, 5, 18, 23, 2000000);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 17, 3, true, 170, 320, 35, 0, 35, 0);

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
bool senalWebActiva = false;
String colaMorse = "";
unsigned long tiempoInicioCola = 0;
bool transmitiendoCola = false;
String textoPendiente = "";

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Telegrafo Morse - Vintage</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Special+Elite&display=swap');
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Special Elite', 'Courier New', monospace;
            background: #1a1714;
            background-image: repeating-linear-gradient(0deg, transparent, transparent 2px, #2a2520 2px, #2a2520 4px);
            color: #c9b896;
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: linear-gradient(180deg, #2d2620 0%, #1f1a16 100%);
            border: 4px solid #4a4035;
            border-radius: 8px;
            padding: 25px;
            box-shadow: inset 0 0 30px rgba(0,0,0,0.5), 0 0 20px rgba(0,0,0,0.8), 0 4px 0 #3d332a;
        }
        h1 { text-align: center; color: #d4af37; text-shadow: 0 0 10px rgba(212,175,55,0.5); font-size: 1.8rem; letter-spacing: 4px; margin-bottom: 5px; border-bottom: 2px dashed #4a4035; padding-bottom: 15px; }
        .subtitle { text-align: center; color: #7a6b56; font-size: 0.75rem; font-style: italic; margin-bottom: 20px; }
        .display { background: #0a0908; border: 3px inset #3d332a; padding: 15px; min-height: 80px; margin-bottom: 15px; font-size: 1.4rem; color: #d4af37; text-shadow: 0 0 5px rgba(212,175,55,0.8); word-break: break-all; letter-spacing: 2px; }
        .display-label { font-size: 0.7rem; color: #5a4f40; margin-bottom: 5px; text-transform: uppercase; letter-spacing: 2px; }
        .panel { background: #252018; border: 2px solid #3d332a; padding: 15px; margin-bottom: 15px; }
        .panel-title { font-size: 0.8rem; color: #7a6b56; border-bottom: 1px solid #4a4035; padding-bottom: 8px; margin-bottom: 12px; text-transform: uppercase; letter-spacing: 3px; }
        .input-row { display: flex; gap: 10px; }
        input[type="text"] { flex: 1; background: #0a0908; border: 2px inset #3d332a; padding: 10px 15px; color: #c9b896; font-family: inherit; font-size: 1rem; }
        button { font-family: inherit; cursor: pointer; }
        .btn-send { background: #3d332a; border: 2px solid #5a4f40; color: #d4af37; padding: 10px 20px; font-size: 0.9rem; letter-spacing: 2px; transition: all 0.1s; }
        .btn-send:hover { background: #4a4035; box-shadow: 0 0 10px rgba(212,175,55,0.3); }
        .btn-send:active { transform: scale(0.98); }
        .telegraph-container { display: flex; flex-direction: column; align-items: center; padding: 20px; }
        .telegraph-label { font-size: 0.7rem; color: #5a4f40; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 3px; }
        .telegraph-btn { width: 120px; height: 120px; border-radius: 50%; background: linear-gradient(145deg, #4a4035, #2d2620); border: 8px solid #3d332a; cursor: pointer; display: flex; align-items: center; justify-content: center; box-shadow: 0 6px 0 #1f1a16, 0 8px 10px rgba(0,0,0,0.5), inset 0 2px 5px rgba(255,255,255,0.1); transition: all 0.1s; }
        .telegraph-btn:active, .telegraph-btn.active { transform: translateY(4px); box-shadow: 0 2px 0 #1f1a16, 0 4px 5px rgba(0,0,0,0.5), inset 0 2px 5px rgba(255,255,255,0.1); background: linear-gradient(145deg, #5a4f40, #3d332a); border-color: #d4af37; }
        .telegraph-btn-inner { width: 60px; height: 60px; border-radius: 50%; background: linear-gradient(145deg, #2d2620, #1f1a16); border: 4px solid #4a4035; box-shadow: inset 0 2px 10px rgba(0,0,0,0.8); }
        .telegraph-btn:active .telegraph-btn-inner, .telegraph-btn.active .telegraph-btn-inner { border-color: #d4af37; box-shadow: inset 0 2px 10px rgba(0,0,0,0.8), 0 0 20px rgba(212,175,55,0.5); background: #d4af37; }
        .status-bar { display: flex; justify-content: space-between; font-size: 0.7rem; color: #5a4f40; margin-top: 20px; border-top: 1px dashed #3d332a; padding-top: 15px; }
        .btn-clear { background: transparent; border: none; color: #7a4a3a; font-family: inherit; font-size: 0.75rem; cursor: pointer; letter-spacing: 1px; }
        .btn-clear:hover { color: #a85a4a; text-decoration: underline; }
        .indicator { width: 12px; height: 12px; border-radius: 50%; background: #2d2620; border: 2px solid #4a4035; display: inline-block; margin-right: 8px; }
        .indicator.on { background: #d4af37; box-shadow: 0 0 10px #d4af37; }
    </style>
</head>
<body>
    <div class="container">
        <h1>TELEGRAFO</h1>
        <p class="subtitle">Sistema de Comunicacion Morse</p>
        <div class="display-label">Codigo Recibido:</div>
        <div class="display" id="morseDisplay">... ... ...</div>
        <div class="display-label">Mensaje Traducido:</div>
        <div class="display" id="textoDisplay">esperando...</div>
        <div class="panel">
            <div class="panel-title">Entrada de Texto</div>
            <div class="input-row">
                <input type="text" id="textInput" placeholder="Escribe tu mensaje..." maxlength="50">
                <button class="btn-send" id="sendBtn">ENVIAR</button>
            </div>
            <div style="margin-top:10px; text-align:right;">
                <button class="btn-clear" id="clearBtn">[LIMPIAR]</button>
            </div>
        </div>
        <div class="telegraph-container">
            <div class="telegraph-label">Pulsar para enviar senales</div>
            <button class="telegraph-btn" id="telegraphBtn"><div class="telegraph-btn-inner"></div></button>
        </div>
        <div class="status-bar">
            <span><span class="indicator" id="indicator"></span><span id="statusText">Conectado</span></span>
            <span>IP: <span id="ipDisplay">---.---.---.---</span></span>
            <span>S: <span id="sensorDisplay">0</span></span>
        </div>
    </div>
    <script>
        const telegraphBtn = document.getElementById('telegraphBtn');
        const indicator = document.getElementById('indicator');
        function actualizar() {
            fetch('/status').then(r => r.json()).then(d => {
                document.getElementById('morseDisplay').textContent = d.morse || '... ... ...';
                document.getElementById('textoDisplay').textContent = d.texto || 'esperando...';
                document.getElementById('ipDisplay').textContent = d.ip;
                document.getElementById('sensorDisplay').textContent = d.sensor;
                if (d.senal === '1') { indicator.classList.add('on'); } else { indicator.classList.remove('on'); }
            }).catch(e => console.log('Error:', e));
        }
        function iniciar() { indicator.classList.add('on'); telegraphBtn.classList.add('active'); fetch('/senal?val=1'); }
        function terminar() { indicator.classList.remove('on'); telegraphBtn.classList.remove('active'); fetch('/senal?val=0'); }
        telegraphBtn.addEventListener('mousedown', iniciar);
        telegraphBtn.addEventListener('touchstart', iniciar);
        window.addEventListener('mouseup', terminar);
        window.addEventListener('touchend', terminar);
        document.getElementById('sendBtn').addEventListener('click', () => {
            let txt = document.getElementById('textInput').value.toUpperCase();
            if (txt) { fetch('/texto?txt=' + encodeURIComponent(txt)); document.getElementById('textInput').value = ''; }
        });
        document.getElementById('clearBtn').addEventListener('click', () => {
            fetch('/limpiar').then(() => { document.getElementById('morseDisplay').textContent = '... ... ...'; document.getElementById('textoDisplay').textContent = 'esperando...'; });
        });
        setInterval(actualizar, 1000);
        actualizar();
    </script>
</body>
</html>
)rawliteral";

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
  return "";
}

String obtenerCodigoMorse(char c) {
  if (c == 'A') return ".-"; if (c == 'B') return "-...";
  if (c == 'C') return "-.-."; if (c == 'D') return "-..";
  if (c == 'E') return "."; if (c == 'F') return "..-.";
  if (c == 'G') return "--."; if (c == 'H') return "....";
  if (c == 'I') return ".."; if (c == 'J') return ".---";
  if (c == 'K') return "-.-"; if (c == 'L') return ".-..";
  if (c == 'M') return "--"; if (c == 'N') return "-.";
  if (c == 'O') return "---"; if (c == 'P') return ".--.";
  if (c == 'Q') return "--.-"; if (c == 'R') return ".-.";
  if (c == 'S') return "..."; if (c == 'T') return "-";
  if (c == 'U') return "..-"; if (c == 'V') return "...-";
  if (c == 'W') return ".--"; if (c == 'X') return "-..-";
  if (c == 'Y') return "-.--"; if (c == 'Z') return "--..";
  return "";
}

void actualizarPantalla() {
  gfx->fillScreen(BLACK);
  gfx->drawRect(0, 0, 320, 40, BLUE); 
  gfx->setCursor(45, 10); gfx->setTextColor(WHITE); gfx->setTextSize(2);
  gfx->println("LI-FI MORSE");

  gfx->setCursor(15, 55); gfx->setTextColor(YELLOW); gfx->setTextSize(2);
  gfx->print("MORSE: "); gfx->println(mensajeMorse);

  gfx->drawFastHLine(0, 100, 320, WHITE); 
  gfx->setCursor(15, 115); gfx->setTextColor(GREEN); gfx->setTextSize(3);
  gfx->println(palabraCompleta);

  gfx->setCursor(10, 150); gfx->setTextSize(1); gfx->setTextColor(BLUE);
  if (WiFi.status() == WL_CONNECTED) {
    gfx->print("IP: "); gfx->print(WiFi.localIP().toString());
  } else {
    gfx->print("Conectando WiFi...");
  }
}

// Tiempos Morse estandar (ajustables)
// Punto: 100ms, Raya: 300ms
// Pausa entre simbolos: 100ms
// Pausa entre letras: 300ms

void procesarCola() {
  if (colaMorse.length() == 0) {
    if (senalWebActiva == false && transmitiendoCola == true) {
      // Transmision terminada - mostrar palabra
      transmitiendoCola = false;
      palabraCompleta += textoPendiente;
      textoPendiente = "";
      actualizarPantalla();
      Serial.println("Palabra: " + palabraCompleta);
    }
    senalWebActiva = false;
    return;
  }
  
  unsigned long tiempo = millis() - tiempoInicioCola;
  char c = colaMorse.charAt(0);
  
  if (senalWebActiva) {
    // Determinar tiempo activo segun simbolo
    unsigned long tiempoActivo = (c == '.') ? 100 : 300;
    if (tiempo >= tiempoActivo) {
      senalWebActiva = false;
      tiempoInicioCola = millis();
    }
  } else {
    // Determinar tiempo de pausa segun simbolo
    unsigned long tiempoPausa = (c == ' ') ? 300 : 100;
    if (tiempo >= tiempoPausa) {
      colaMorse = colaMorse.substring(1);
      if (colaMorse.length() > 0) {
        c = colaMorse.charAt(0);
        if (c == '.' || c == '-') {
          senalWebActiva = true;
          tiempoInicioCola = millis();
        }
      }
    }
  }
}

void iniciarTransmision() {
  if (colaMorse.length() > 0) {
    char c = colaMorse.charAt(0);
    if (c == '.') senalWebActiva = true;
    else if (c == '-') senalWebActiva = true;
    else if (c == ' ') senalWebActiva = false;
    tiempoInicioCola = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando...");

  // Inicializar pantalla
  if (!gfx->begin()) { Serial.println("Error Pantalla"); }
  Serial.println("Pantalla OK");
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);
  gfx->setCursor(10, 120); gfx->setTextColor(WHITE); gfx->setTextSize(2);
  gfx->println("Iniciando WiFi...");
  Serial.println("Antes WiFi");
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);

  // Configurar pines
  pinMode(sensorPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Esperar conexión
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 60) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.println("IP: " + WiFi.localIP().toString());
    gfx->setCursor(10, 140); gfx->setTextColor(GREEN); gfx->setTextSize(2);
    gfx->println("WiFi OK!");
    delay(500);
  } else {
    Serial.println("\nError WiFi!");
    gfx->setCursor(10, 140); gfx->setTextColor(RED); gfx->setTextSize(2);
    gfx->println("Error WiFi!");
  }

  server.on("/", [](){
    server.send(200, "text/html", INDEX_HTML);
  });

  server.on("/senal", HTTP_GET, [](){
    if(server.hasArg("val")) {
      String valStr = server.arg("val");
      int val = valStr.toInt();
      Serial.println("senal: " + valStr);
      if (val == 1) senalWebActiva = true;
      else senalWebActiva = false;
    }
    server.send(200, "text/plain", "ok");
  });

  server.on("/status", HTTP_GET, [](){
    int sensor = analogRead(sensorPin);
    bool senal = (sensor > umbral) || senalWebActiva;
    String json = "{\"morse\":\"" + mensajeMorse + "\",\"texto\":\"" + palabraCompleta + "\",\"sensor\":" + sensor + ",\"senal\":\"" + (senal ? "1" : "0") + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.send(200, "application/json", json);
  });

  server.on("/agregar", HTTP_GET, [](){
    if(server.hasArg("morse")) {
      String m = server.arg("morse");
      mensajeMorse += m;
      palabraCompleta += traducirMorse(m);
      mensajeMorse = "";
      actualizarPantalla();
    }
    server.send(200, "text/plain", "ok");
  });

  server.on("/limpiar", HTTP_GET, [](){
    mensajeMorse = "";
    palabraCompleta = "";
    actualizarPantalla();
    server.send(200, "text/plain", "ok");
  });

  server.on("/texto", HTTP_GET, [](){
    if(server.hasArg("txt")) {
      String txt = server.arg("txt");
      String txtupper = txt;
      txtupper.toUpperCase();
      Serial.println("Texto recibido: " + txtupper);
      textoPendiente = txtupper;
      colaMorse = "";
      for(int i = 0; i < txtupper.length(); i++) {
        char c = txtupper.charAt(i);
        if(c == ' ') { 
          colaMorse += " ";
        }
        else {
          String codigo = obtenerCodigoMorse(c);
          if(codigo != "") {
            colaMorse += codigo;
          }
        }
      }
      mensajeMorse = "";
      Serial.println("colaMorse: " + colaMorse);
      transmitiendoCola = true;
      iniciarTransmision();
      actualizarPantalla();
    }
    server.send(200, "text/plain", "ok");
  });

  Serial.println("Inicio server...");
  server.begin();
  Serial.println("Server iniciado");
  actualizarPantalla();
  Serial.println("Setup completo");
}

unsigned long ultimoCheckWiFi = 0;

void loop() {
  server.handleClient();
  procesarCola();

  if (millis() - ultimoCheckWiFi > 30000) {
    ultimoCheckWiFi = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi desconectado, reconnecting...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(ssid, password);
    }
  }

  int valorLuz = analogRead(sensorPin);
  bool señalActiva = (valorLuz > umbral) || senalWebActiva;

  if (señalActiva) {
    tone(speakerPin, hz);
    digitalWrite(greenPin, HIGH);
    if (!transmitiendo) { tiempoInicio = millis(); transmitiendo = true; espacioAgregado = false; }
  } else {
    noTone(speakerPin);
    digitalWrite(greenPin, LOW);
    if (transmitiendo) {
      unsigned long duracion = millis() - tiempoInicio;
      transmitiendo = false;
      tiempoApagado = millis();
      // Punto: < 200ms, Raya: >= 200ms (estandar Morse)
      if (duracion > 30 && duracion < 200) mensajeMorse += ".";
      else if (duracion >= 200) mensajeMorse += "-";
      actualizarPantalla();
    }
  }

  if (!transmitiendo && (millis() - tiempoApagado > 400) && mensajeMorse != "") {
    palabraCompleta += traducirMorse(mensajeMorse);
    if (mensajeMorse == ".....") {
      palabraCompleta = "";
    }
    mensajeMorse = "";
    digitalWrite(redPin, HIGH); delay(50); digitalWrite(redPin, LOW);
    actualizarPantalla();
  }

  if (!transmitiendo && (millis() - tiempoApagado > 1200) && !espacioAgregado && palabraCompleta != "") {
    palabraCompleta += " ";
    espacioAgregado = true;
    actualizarPantalla();
  }

  if (!transmitiendo && (millis() - tiempoApagado > 10000) && palabraCompleta != "") {
    palabraCompleta = "";
    mensajeMorse = "";
    actualizarPantalla();
  }
}
