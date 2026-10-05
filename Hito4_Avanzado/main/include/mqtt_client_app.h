#ifndef MQTT_CLIENT_APP_H
#define MQTT_CLIENT_APP_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el cliente MQTT y establece conexión con ThingsBoard.
 * @param[in] broker_uri Dirección URI del broker MQTT.
 * @param[in] access_token Token de acceso del dispositivo en ThingsBoard.
 * @return esp_err_t ESP_OK si la pila inicializó correctamente.
 */
esp_err_t mqtt_client_app_init(const char *broker_uri, const char *access_token);

/**
 * @brief Verifica si existe una conexión TCP/MQTT activa con el broker.
 */
bool mqtt_client_app_is_connected(void);

/**
 * @brief Publica una carga útil en formato JSON hacia el topic de telemetría de ThingsBoard.
 * @param[in] json_payload Cadena formateada en JSON (ej. "{\"lux\": 250.0}").
 * @return int ID del mensaje publicado o -1 en caso de error.
 */
int mqtt_client_app_send_telemetry(const char *json_payload);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_APP_H