#include <Arduino.h>
#include "src/Estacion.h"
#include "src/Red.h"

const long INTERVALO_POT = 50;
const long INTERVALO_DHT = 2200;
const long INTERVALO_OLED = 250;
long ultimoPot = 0, ultimoDht = 0, ultimoOled = 0;

void setup() {
  Serial.begin(115200);
  estacion::iniciar();
  red::iniciar();
}

void loop() {
  const long ahora = millis();
  if (ahora - ultimoPot >= INTERVALO_POT) {
    ultimoPot = ahora;
    estacion::leerPotenciometro();
  }
  if (ahora - ultimoDht >= INTERVALO_DHT) {
    ultimoDht = ahora;
    estacion::leerDht();
  }
  if (ahora - ultimoOled >= INTERVALO_OLED) {
    ultimoOled = ahora;
    estacion::actualizarOled();
  }
  red::actualizar();
  delay(10);
}
