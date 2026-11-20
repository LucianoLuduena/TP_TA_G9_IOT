#pragma once
#include <Arduino.h>

namespace estacion {
void iniciar();
void leerPotenciometro();
void leerDht();
void actualizarOled();
// Ejecuta un comando ya normalizado y devuelve el mensaje para Telegram.
String comando(const String& texto);
bool obtenerClima(float& temperatura, float& humedad);
long marcaHistorial();
void registrarEnvioManual(long marcaAlRecibir);
}
