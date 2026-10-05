#include "mqtt_client_app.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "app_config.h"

static const char *TAG = "TB_MQTT";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool                     s_is_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_is_connected = true;
        ESP_LOGI(TAG, "==================================================");
        ESP_LOGI(TAG, "CONECTADO CON EXITO AL BROKER DE THINGSBOARD");
        ESP_LOGI(TAG, "Listo para transferir telemetria");
        ESP_LOGI(TAG, "==================================================");
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_is_connected = false;
        ESP_LOGW(TAG, "Desconectado del broker MQTT");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Telemetria entregada a ThingsBoard (msg_id=%d)", event->msg_id);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error en comunicacion MQTT");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_client_app_init(const char *broker_uri, const char *access_token)
{
    if (broker_uri == NULL || access_token == NULL) {
        ESP_LOGE(TAG, "Parametros URI o Access Token invalidos");
        return ESP_ERR_INVALID_ARG;
    }

    // Estructura oficial de configuración para ESP-IDF v5.x
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .credentials.username = access_token, // En ThingsBoard el Access Token actúa como usuario
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Fallo al inicializar cliente MQTT");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));

    ESP_LOGI(TAG, "Cliente MQTT iniciado hacia: %s", broker_uri);
    return ESP_OK;
}

bool mqtt_client_app_is_connected(void)
{
    return s_is_connected;
}

int mqtt_client_app_send_telemetry(const char *json_payload)
{
    if (s_mqtt_client == NULL || !s_is_connected) {
        ESP_LOGW(TAG, "No se puede publicar: MQTT no conectado aun");
        return -1;
    }

    // QoS 1 garantiza entrega en la plataforma IoT
    return esp_mqtt_client_publish(s_mqtt_client, TB_TELEMETRY_TOPIC, json_payload, 0, 1, 0);
}