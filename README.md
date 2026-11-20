# Estación ambiental ESP32 · TA 4k01

## Como funciona

1. Copiar `src/Secretos.ejemplo.h` como `src/Secretos.h`.
2. Completar en `src/Secretos.h` el nombre y la contraseña del Wi-Fi, el token del bot de Telegram y la clave de escritura de ThingSpeak. Indicá también el ID del canal si corresponde.
3. Instalar las bibliotecas necesarias: DHT sensor library, Adafruit SH110X, UniversalTelegramBot, ArduinoJson 6.21.5 y ThingSpeak 2.1.1, junto con sus dependencias.
4. Conectá los componentes según el cableado del proyecto.

## Archivos principales

| Archivo | Función |
|---|---|
| `Integrador.ino` | Programa principal: inicializa la estación y coordina sus tareas. |
| `src/main.cpp` | Permite que PlatformIO compile el sketch `Integrador.ino`. |
| `src/Estacion.h` | Declara las funciones de la estación. |
| `src/Estacion.cpp` | Maneja el sensor, el potenciómetro, los LEDs y la pantalla. |
| `src/Red.h` | Declara las funciones de conexión y comunicación de red. |
| `src/Red.cpp` | Gestiona Wi-Fi, Telegram y ThingSpeak. |
| `src/Secretos.ejemplo.h` | Plantilla de configuración; copiar como `Secretos.h` y completar. |
| `src/Secretos.h` | Credenciales y datos privados de los servicios. No compartir este archivo. |

## Elegir el entorno

En PlatformIO, seleccionar `wokwi` para el simulador o `esp32doit-devkit-v1` para la placa real. El entorno `wokwi` agrega la macro `ENTORNO_WOKWI`, que selecciona la pantalla SSD1306; sin ella se usa la pantalla SH1106 de la placa real. Para Wokwi se configura automáticamente la red `Wokwi-GUEST`.

En la placa de protoboard, el sensor DHT se elige con GPIO27: conectarlo a GND para usar el DHT11 de la presentación o dejarlo libre para usar el DHT22 de la placa de Pedroni.
