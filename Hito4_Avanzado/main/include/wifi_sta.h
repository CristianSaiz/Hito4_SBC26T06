#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bits de sincronización del EventGroup de Wi-Fi */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**
 * @brief Inicializa la interfaz Wi-Fi en modo Estación (STA) con reconexión automática.
 * 
 * Configura LwIP, el bucle de eventos del sistema, registra los manejadores de
 * WIFI_EVENT e IP_EVENT y lanza la conexión hacia las credenciales dadas.
 * 
 * @param[in] ssid     Nombre de la red Wi-Fi (SSID).
 * @param[in] password Contraseña WPA/WPA2 de la red Wi-Fi.
 * @return esp_err_t   ESP_OK si la pila de red arrancó correctamente.
 */
esp_err_t wifi_sta_init(const char *ssid, const char *password);

/**
 * @brief Bloquea la tarea llamante hasta que el ESP32 obtenga IP o falle.
 * 
 * @param[in] timeout_ms Tiempo máximo de espera en milisegundos (o portMAX_DELAY).
 * @return EventBits_t   Bits activos (WIFI_CONNECTED_BIT o WIFI_FAIL_BIT).
 */
EventBits_t wifi_sta_wait_connected(uint32_t timeout_ms);

/**
 * @brief Comprueba si el dispositivo tiene actualmente conectividad IP establecida.
 * 
 * @return true si la interfaz STA tiene una IP asignada por DHCP.
 */
bool wifi_sta_is_connected(void);

/**
 * @brief Obtiene la dirección IP actual en formato de cadena de texto (IPv4).
 * 
 * @param[out] ip_str Búfer donde se copiará la IP (mínimo 16 bytes: "xxx.xxx.xxx.xxx").
 * @param[in]  max_len Tamaño del búfer provisto.
 */
void wifi_sta_get_ip_string(char *ip_str, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // WIFI_STA_H