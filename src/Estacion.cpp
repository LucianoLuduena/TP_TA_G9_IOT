#include "Estacion.h"
#ifdef ENTORNO_WOKWI
#include <Adafruit_SSD1306.h>
#else
#include <Adafruit_SH110X.h>
#endif
#include <DHT.h>
#include <WiFi.h>
#include <Wire.h>

namespace {
const int PIN_DHT = 33, PIN_TIPO_DHT = 27;
const int PIN_POT = 32, PIN_VERDE = 23, PIN_AZUL = 2;
constexpr float UMBRAL_INICIAL = 0.5f;
const int LINEAS = 3;
enum class Vista { Clima, Leds, Pot, Desconocido };
DHT sensorDHT11(PIN_DHT, DHT11);
DHT sensorDHT22(PIN_DHT, DHT22);
DHT* sensor = nullptr;
const char* modeloDHT = "DHT22";
const int DIR_OLED = 0x3C;
#ifdef ENTORNO_WOKWI
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
const char* modeloOLED = "SSD1306";
bool iniciarOLED() { return oled.begin(SSD1306_SWITCHCAPVCC, DIR_OLED, false, false); }
#else
Adafruit_SH1106G oled(128, 64, &Wire, -1, 100000, 100000);
const char* modeloOLED = "SH1106";
bool iniciarOLED() { return oled.begin(DIR_OLED, false); }
#endif
bool conPantalla = false;
float temperatura = NAN, humedad = NAN, voltios = 0, umbral = UMBRAL_INICIAL;
bool automatico = false, verde = false, azul = false;
Vista vista = Vista::Clima;
String historial[LINEAS];
long totalRegistrado = 0;

void registrar(const String& texto, long posicion = 0) {
  ++totalRegistrado;
  if (posicion >= LINEAS) return;
  for (int i = LINEAS - 1; i > posicion; --i) historial[i] = historial[i - 1];
  historial[posicion] = texto.substring(0, 21);  // En 128 px caben 21 caracteres.
}

void aplicarSalidas() {
  if (automatico) azul = voltios > umbral;
  digitalWrite(PIN_VERDE, verde ? HIGH : LOW);
  digitalWrite(PIN_AZUL, azul ? HIGH : LOW);
}

const char* encendido(bool activo) { return activo ? "encendido" : "apagado"; }

void dibujar() {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(WiFi.status() == WL_CONNECTED ? "WiFi: conectado" : "WiFi: desconectado");
  switch (vista) {
    case Vista::Leds:
      oled.println("ACTUADORES");
      oled.printf("Verde: %s\nAzul:  %s", encendido(verde), encendido(azul));
      break;
    case Vista::Pot:
      oled.println("POTENCIOMETRO");
      oled.printf("Lectura: %.2f V\nUmbral:  %.2f V", voltios, umbral);
      break;
    case Vista::Clima:
      oled.printf("AMBIENTE / %s\n", modeloDHT);
      if (isnan(temperatura) || isnan(humedad)) oled.print("Sin lectura valida");
      else oled.printf("Temp: %.1f C\nHumedad: %.1f %%", temperatura, humedad);
      break;
    case Vista::Desconocido:
      oled.println("DISPLAY");
      oled.print("Comando desconocido");
      break;
  }
  oled.setCursor(0, 32);
  oled.println("Ultimos comandos:");
  for (const auto& linea : historial) oled.println(linea);
  oled.display();
}

const char MENU[] =
    "🌡 <b>Estación UTN</b>\nEscribí un comando por mensaje.\n\n"
    "📊 <b>Mediciones</b>\n/estado · consultar la estación\n"
    "/thingup · publicar temperatura y humedad\n\n"
    "💡 <b>Control</b>\n/autoon · usar el voltaje actual como umbral\n"
    "/autooff · pasar a manual\n/umbral 1.25 · fijar entre 1 y 1.5 V\n"
    "/verdeon /verdeoff · LED verde\n/azulon /azuloff · LED azul (manual)\n\n"
    "🖥 <b>Pantalla</b>\n/displayled · LEDs\n/displaypot · potenciómetro\n"
    "/displaydht22 · temperatura y humedad";
}

namespace estacion {
void iniciar() {
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_AZUL, OUTPUT);
  pinMode(PIN_POT, INPUT);
  aplicarSalidas();
  analogReadResolution(12);
  pinMode(PIN_TIPO_DHT, INPUT_PULLUP);
  const bool seleccionarDHT11 = digitalRead(PIN_TIPO_DHT) == LOW;
  sensor = seleccionarDHT11 ? &sensorDHT11 : &sensorDHT22;
  modeloDHT = seleccionarDHT11 ? "DHT11" : "DHT22";
  sensor->begin();
  Serial.printf("Sensor seleccionado: %s (GPIO27 %s)\n", modeloDHT,
                seleccionarDHT11 ? "conectado a GND" : "libre");
  Wire.begin(21, 22);
  Wire.beginTransmission(DIR_OLED);
  conPantalla = Wire.endTransmission() == 0 && iniciarOLED();
  if (conPantalla) {
    oled.setTextSize(1);
    oled.setTextColor(1); // Blanco en ambos controladores monocromos.
    oled.setTextWrap(false);
    dibujar();
    Serial.printf("OLED %s inicializado en 0x%02X\n", modeloOLED, DIR_OLED);
  } else Serial.println("OLED ausente en 0x3C");
}

void leerPotenciometro() {
  const int lecturaADC = analogRead(PIN_POT);
  const float voltaje = (lecturaADC * 3.3f) / 4095;
  voltios = voltaje;
  aplicarSalidas();
}

void leerDht() {
  const float t = sensor->readTemperature();
  const float h = sensor->readHumidity();
  temperatura = t;
  humedad = h;
}

void actualizarOled() {
  if (!conPantalla) return;
  dibujar();
}

bool obtenerClima(float& t, float& h) {
  t = temperatura;
  h = humedad;
  return !isnan(t) && !isnan(h);
}

long marcaHistorial() {
  return totalRegistrado;
}

void registrarEnvioManual(long marcaAlRecibir) {
  // Los comandos recibidos durante la espera deben quedar por encima de /thingup.
  registrar("/thingup", totalRegistrado - marcaAlRecibir);
}

String comando(const String& texto) {
  String respuesta;
  if (texto == "/start") respuesta = MENU;
  else if (texto == "/estado") {
    if (isnan(temperatura) || isnan(humedad)) respuesta = String(modeloDHT) + ": sin lectura válida";
    else respuesta = "Temperatura: " + String(temperatura, 1) + " °C\nHumedad: " + String(humedad, 1) + " %";
    respuesta += "\nPotenciómetro: " + String(voltios, 2) + " V\nUmbral: " + String(umbral, 2) + " V";
    respuesta += String("\nVerde: ") + encendido(verde) + "\nAzul: " + encendido(azul);
    respuesta += automatico ? "\nModo automático" : "\nModo manual";
  } else if (texto == "/autoon") {
    automatico = true;
    umbral = voltios;
    respuesta = "Automático. Referencia capturada: " + String(umbral, 2) + " V.";
  } else if (texto == "/autooff") {
    automatico = false;
    respuesta = "Modo manual. Los LEDs conservan su estado.";
  } else if (texto == "/umbral" || texto.startsWith("/umbral ")) {
    String argumento = texto.substring(7);
    argumento.trim();
    argumento.replace(',', '.');
    char* fin;
    const double valor = strtod(argumento.c_str(), &fin);
    if (fin == argumento.c_str() || *fin || !(valor >= 1 && valor <= 1.5)) {
      umbral = UMBRAL_INICIAL;
      aplicarSalidas();
      return "Valor inválido: se restableció el umbral a 0.50 V.";
    }
    umbral = valor;
    respuesta = "Umbral: " + String(umbral, 2) + " V.";
  } else if (texto == "/azulon" || texto == "/azuloff") {
    if (automatico) return "El azul está en automático. Usá /autooff primero.";
    azul = texto == "/azulon";
    respuesta = String("LED azul ") + encendido(azul);
  } else if (texto == "/verdeon" || texto == "/verdeoff") {
    verde = texto == "/verdeon";
    respuesta = String("LED verde ") + encendido(verde);
  } else if (texto.startsWith("/display")) {
    if (texto == "/displayled") vista = Vista::Leds;
    else if (texto == "/displaypot") vista = Vista::Pot;
    else if (texto == "/displaydht22" || texto == "/displaydht") vista = Vista::Clima;
    else {
      vista = Vista::Desconocido;
      return "Comando de display desconocido.";
    }
    respuesta = conPantalla ? "Vista del OLED actualizada." : "Vista seleccionada. OLED no detectado.";
  } else return "Comando desconocido. Consultá /start.";
  aplicarSalidas();
  registrar(texto);
  return respuesta;
}
}
