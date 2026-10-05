#ifndef SENSOR_ADC_H
#define SENSOR_ADC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Ventana de 10 muestras a 2 segundos de cadencia = 20 segundos de media movil
#define VENTANA_MEDIA_MOVIL  10  

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configura la unidad ADC1 (GPIO 34) e inicializa calibracion eFuse.
 */
esp_err_t sensor_adc_init(void);

/**
 * @brief Realiza una lectura Oneshot y devuelve el valor raw y los mV calibrados.
 */
esp_err_t sensor_adc_leer(int *raw_out, int *mv_out);

/**
 * @brief Convierte la lectura en milivoltios a valor estimado de Lux.
 */
float sensor_adc_calcular_lux(int mv);

/**
 * @brief Inserta una nueva muestra en el buffer circular y retorna la media movil.
 * @param[in] nuevo_lux Valor instantaneo leido de la LDR.
 * @return float Media movil de la ventana temporal (ultimos 20 segundos).
 */
float sensor_adc_actualizar_media(float nuevo_lux);

/**
 * @brief Consulta la media movil calculada actualmente sin insertar muestras.
 */
float sensor_adc_obtener_media(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_ADC_H