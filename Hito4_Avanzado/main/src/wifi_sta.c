#include "wifi_sta.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "WIFI_STA";

#define MAXIMUM_RETRY 5

static EventGroupHandle_t s_wifi_event_group = NULL;
static esp_netif_t       *s_sta_netif        = NULL;
static int                s_retry_num        = 0;
static bool               s_is_connected     = false;
static char               s_ip_addr_str[16]  = "0.0.0.0";

/**
 * @brief Manejador del Event Loop del sistema para eventos de Wi-Fi e IP.
 */
static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Driver Wi-Fi iniciado. Conectando al AP...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_is_connected = false;
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Reintentando conexion al AP (%d/%d)...", s_retry_num, MAXIMUM_RETRY);
        } else {
            ESP_LOGE(TAG, "No fue posible conectar al AP tras %d reintentos.", MAXIMUM_RETRY);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_addr_str, sizeof(s_ip_addr_str), IPSTR, IP2STR(&event->ip_info.ip));
        
        ESP_LOGI(TAG, "==================================================");
        ESP_LOGI(TAG, "Conexion Wi-Fi STA establecida con exito");
        ESP_LOGI(TAG, "Direccion IP asignada: %s", s_ip_addr_str);
        ESP_LOGI(TAG, "==================================================");

        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_sta_init(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "SSID o password invalidos (NULL)");
        return ESP_ERR_INVALID_ARG;
    }

    s_retry_num = 0;
    s_is_connected = false;

    // 1. Crear el grupo de eventos si no existe
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL) {
            ESP_LOGE(TAG, "Fallo al instanciar el EventGroup de FreeRTOS");
            return ESP_ERR_NO_MEM;
        }
    }
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // 2. Inicializar subsistema LwIP y bucle de eventos por defecto
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    // 3. Crear la interfaz por defecto para STA una sola vez (evita panics en LwIP)
    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        assert(s_sta_netif != NULL);
    }

    // 4. Inicializar driver Wi-Fi con configuración estándar
    static bool s_driver_initialized = false;
    if (!s_driver_initialized) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Registrar escuchadores de eventos
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_sta_event_handler,
            NULL,
            NULL
        ));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_sta_event_handler,
            NULL,
            NULL
        ));

        s_driver_initialized = true;
    }

    // 5. Configurar credenciales y arrancar la interfaz
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Configuracion Wi-Fi STA completada para SSID: '%s'", ssid);
    return ESP_OK;
}

EventBits_t wifi_sta_wait_connected(uint32_t timeout_ms)
{
    if (s_wifi_event_group == NULL) {
        return 0;
    }

    TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,   // No limpiar bits al salir
        pdFALSE,   // Salir si cualquiera de los dos bits se activa
        ticks
    );
}

bool wifi_sta_is_connected(void)
{
    return s_is_connected;
}

void wifi_sta_get_ip_string(char *ip_str, size_t max_len)
{
    if (ip_str && max_len > 0) {
        strncpy(ip_str, s_ip_addr_str, max_len - 1);
        ip_str[max_len - 1] = '\0';
    }
}