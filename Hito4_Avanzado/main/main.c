#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "wifi_sta.h"
#include "mqtt_client_app.h"
#include "sensor_adc.h"
#include "actuators.h"

static const char *TAG = "APP_HITO4_ADV";

static void tarea_telemetria(void *pvParameters)
{
    char json_buffer[128];
    int raw = 0;
    int mv = 0;

    while (1) {
        // Cadencia estandar de 2 segundos (10 muestras = ventana de 20s para la media movil)
        vTaskDelay(pdMS_TO_TICKS(2000));

        bool conectado = mqtt_client_app_is_connected();

        if (sensor_adc_leer(&raw, &mv) == ESP_OK) {
            float lux_inst = sensor_adc_calcular_lux(mv);
            float lux_avg  = sensor_adc_actualizar_media(lux_inst);

            // 1. Display de 7 segmentos: Nivel instantaneo 0 a 9
            int nivel_luz = (int)((lux_inst / 1000.0f) * 10.0f);
            if (nivel_luz > 9) nivel_luz = 9;
            if (nivel_luz < 0) nivel_luz = 0;
            actuadores_mostrar_nivel_7seg(nivel_luz);

            // 2. Pantalla OLED: Telemetria y grafico en scroll continuo
            actuadores_actualizar_oled(lux_inst, mv, conectado);

            // 3. Telemetria avanzada a ThingsBoard: instantaneo + media movil
            if (conectado) {
                snprintf(json_buffer, sizeof(json_buffer), 
                         "{\"lux\":%.1f,\"lux_avg\":%.1f,\"voltaje_mv\":%d}", 
                         lux_inst, lux_avg, mv);

                ESP_LOGI(TAG, "Publicando -> %s (Nivel: %d)", json_buffer, nivel_luz);
                mqtt_client_app_send_telemetry(json_buffer);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Hito 4 Avanzado (Media Movil / Edge Computing)");

    // 1. Almacenamiento no volatil NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Perifericos locales
    ESP_ERROR_CHECK(sensor_adc_init());
    ESP_ERROR_CHECK(actuadores_init());

    // 3. Conexion Wi-Fi STA
    ESP_ERROR_CHECK(wifi_sta_init(WIFI_SSID, WIFI_PASS));
    wifi_sta_wait_connected(15000);

    // 4. Conexion MQTT con ThingsBoard
    ESP_ERROR_CHECK(mqtt_client_app_init(TB_BROKER_URI, TB_ACCESS_TOKEN));

    // 5. Tarea concurrente de telemetria
    xTaskCreate(tarea_telemetria, "tarea_telemetria", 4096, NULL, 5, NULL);
}