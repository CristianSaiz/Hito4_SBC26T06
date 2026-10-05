#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <stdbool.h>
#include "esp_err.h"

// Inicializa los pines del display de 7 segmentos y la pantalla OLED
esp_err_t actuadores_init(void);

// Actualiza el 7 segmentos con un digito de 0 a 9 segun el nivel de luz
void actuadores_mostrar_nivel_7seg(int nivel);

// Actualiza la pantalla OLED con los luxes, tension y estado de red
void actuadores_actualizar_oled(float lux, int mv, bool conectado_mqtt);

#endif // ACTUATORS_H