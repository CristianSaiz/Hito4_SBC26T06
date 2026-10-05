#ifndef SENSOR_ADC_H
#define SENSOR_ADC_H

#include "esp_err.h"

// Inicializa el ADC1 en GPIO 34 con atenuacion de 12dB y calibracion eFuse
esp_err_t sensor_adc_init(void);

// Lee el valor bruto (0-4095) y los milivoltios calculados
esp_err_t sensor_adc_leer(int *raw_out, int *mv_out);

// Convierte la lectura de milivoltios a un valor estimado de luxes
float sensor_adc_calcular_lux(int mv);

#endif // SENSOR_ADC_H