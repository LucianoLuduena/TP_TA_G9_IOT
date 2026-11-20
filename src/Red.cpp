#include "Red.h"
#include "Estacion.h"
#if __has_include("Secretos.h")
#include "Secretos.h"
#else
#include "Secretos.ejemplo.h"
#endif
#include <WiFi.h>
#include <WiFiClientSecure.h>
#define TS_ENABLE_SSL
#include <ThingSpeak.h>
#include <UniversalTelegramBot.h>
#include <time.h>

#ifndef THINGSPEAK_CHANNEL_ID
#define THINGSPEAK_CHANNEL_ID 0
#endif

namespace {
WiFiClientSecure clienteBot;
WiFiClientSecure clienteThingSpeak;
UniversalTelegramBot bot(BOT_TOKEN, clienteBot);
const long PERIODO_MS = 60000, PAUSA_ENTRE_ENVIOS_MS = 16000;
long ultimoIntento = 0;
bool huboIntento = false;
String chatEnvioPendiente;
long marcaAlPedirEnvio = 0;

String publicar(bool& exito) {
  float t, h;
  exito = false;
  if (!estacion::obtenerClima(t, h)) return "Sin lectura válida del DHT22; no se publicó.";
  ThingSpeak.setField(1, t);
  ThingSpeak.setField(2, h);
  const int codigo = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_KEY);
  exito = codigo == TS_OK_SUCCESS;
  if (exito) return "ThingSpeak: medición publicada.";
  return "ThingSpeak no confirmó la escritura (código " + String(codigo) + ").";
}

void atenderTelemetria() {
  if (!THINGSPEAK_WRITE_KEY[0]) return;
  long transcurrido = millis() - ultimoIntento;
  if (huboIntento && transcurrido < PAUSA_ENTRE_ENVIOS_MS) return;
  if (chatEnvioPendiente.length() == 0 && transcurrido < PERIODO_MS) return;
  bool exito;
  String resultado = publicar(exito);
  ultimoIntento = millis();
  huboIntento = true;
  Serial.println(resultado);
  if (chatEnvioPendiente.length() > 0) {
    if (exito) estacion::registrarEnvioManual(marcaAlPedirEnvio);
    bot.sendMessage(chatEnvioPendiente, resultado, "");
    chatEnvioPendiente = "";
  }
}

void procesarMensajes(int cantidad) {
  for (int i = 0; i < cantidad; ++i) {
    const String& chat = bot.messages[i].chat_id;
    if (bot.messages[i].type != "message" || chat.length() == 0) continue;
    String texto = bot.messages[i].text;
    texto.trim();
    texto.toLowerCase();
    if (texto == "/thingup") {
      if (!THINGSPEAK_WRITE_KEY[0]) {
        bot.sendMessage(chat, "Falta la Write API Key.", "");
      } else if (chatEnvioPendiente.length() > 0) {
        bot.sendMessage(chat, "Ya hay un envío pendiente. Volvé a pedirlo en unos segundos.", "");
      } else {
        marcaAlPedirEnvio = estacion::marcaHistorial();
        chatEnvioPendiente = chat;
        if (huboIntento && millis() - ultimoIntento < PAUSA_ENTRE_ENVIOS_MS)
          bot.sendMessage(chat, "Envío pendiente. Te aviso cuando se publique.", "");
        atenderTelemetria();
      }
    } else {
      String respuesta = estacion::comando(texto);
      bot.sendMessage(chat, respuesta, texto == "/start" ? "HTML" : "");
    }
  }
}

void atenderTelegram() {
  int cantidad = bot.getUpdates(bot.last_message_received + 1);
  while (cantidad > 0) {
    procesarMensajes(cantidad);
    atenderTelemetria();
    cantidad = bot.getUpdates(bot.last_message_received + 1);
  }
}

}

void red::iniciar() {
  if (!BOT_TOKEN[0]) Serial.println("Bot deshabilitado: falta el token.");
  if (!THINGSPEAK_WRITE_KEY[0]) Serial.println("ThingSpeak deshabilitado: falta clave de escritura.");
  clienteBot.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  // HTTPS sin verificar el certificado del servidor ThingSpeak.
  clienteThingSpeak.setInsecure();
  ThingSpeak.begin(clienteThingSpeak);
  WiFi.mode(WIFI_STA);
#ifdef ENTORNO_WOKWI
  WiFi.begin("Wokwi-GUEST", "");
#else
  if (!WIFI_SSID[0]) {
    Serial.println("Configurá la red de la placa en src/Secretos.h.");
    return;
  }
  WiFi.begin(WIFI_SSID, WIFI_CLAVE);
#endif
  configTime(0, 0, "pool.ntp.org", "time.google.com");
}

void red::actualizar() {
  static long ultimaConsulta = 0;
  static long ultimaConexion = 0;
  static bool conectadoAntes = false;
  long ahora = millis();
  bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado && !conectadoAntes) Serial.println("Wi-Fi conectado. Sincronizando hora para HTTPS.");
  conectadoAntes = conectado;
  if (!conectado) {
    if (ahora - ultimaConexion >= 15000) {
      ultimaConexion = ahora;
      WiFi.reconnect();
    }
    return;
  }
  if (time(nullptr) <= 1700000000) return;
  if (BOT_TOKEN[0] && ahora - ultimaConsulta >= 1000) {
    atenderTelegram();
    ultimaConsulta = ahora;
  }
  atenderTelemetria();
}

