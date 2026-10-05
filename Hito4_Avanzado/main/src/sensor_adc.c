#include "sensor_adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_ADC";

// Pin GPIO 34 (canal 6 del ADC1)
#define CANAL_LDR          ADC_CHANNEL_6
#define UNIDAD_ADC         ADC_UNIT_1
#define ATENUACION_LDR     ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool calibrado = false;

// Variables internas del Buffer Circular (Edge Computing)
static float s_buffer_muestras[VENTANA_MEDIA_MOVIL] = {0};
static int   s_indice_escritura = 0;
static int   s_total_muestras = 0;
static float s_suma_acumulada = 0.0f;

esp_err_t sensor_adc_init(void)
{
    // 1. Configuracion de la unidad ADC1 Oneshot
    adc_oneshot_unit_init_cfg_t config_unidad = {
        .unit_id = UNIDAD_ADC,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&config_unidad, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear la unidad ADC: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Canal 6 a 12 bits de resolucion
    adc_oneshot_chan_cfg_t config_canal = {
        .atten = ATENUACION_LDR,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CANAL_LDR, &config_canal));

    // 3. Calibracion de fabrica por eFuse
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t config_cali = {
        .unit_id = UNIDAD_ADC,
        .atten = ATENUACION_LDR,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&config_cali, &cali_handle) == ESP_OK) {
        calibrado = true;
        ESP_LOGI(TAG, "Calibracion eFuse cargada con exito");
    } else {
        ESP_LOGW(TAG, "Sin calibracion eFuse, usando aproximacion lineal");
    }
#endif

    // Reiniciar buffer de media movil
    s_indice_escritura = 0;
    s_total_muestras = 0;
    s_suma_acumulada = 0.0f;

    return ESP_OK;
}

esp_err_t sensor_adc_leer(int *raw_out, int *mv_out)
{
    if (!adc_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    int lectura_raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, CANAL_LDR, &lectura_raw);
    if (err != ESP_OK) {
        return err;
    }

    if (raw_out) {
        *raw_out = lectura_raw;
    }

    if (mv_out) {
        if (calibrado) {
            adc_cali_raw_to_voltage(cali_handle, lectura_raw, mv_out);
        } else {
            *mv_out = (lectura_raw * 3300) / 4095;
        }
    }

    return ESP_OK;
}

float sensor_adc_calcular_lux(int mv)
{
    if (mv < 50) mv = 50;
    if (mv > 3250) mv = 3250;

    float v_out = (float)mv / 1000.0f;
    float r_ldr = (3.3f - v_out) * 10000.0f / v_out;

    float lux = 500.0f / (r_ldr / 1000.0f);
    if (lux < 0.0f) lux = 0.0f;
    if (lux > 1500.0f) lux = 1500.0f;

    return lux;
}

float sensor_adc_actualizar_media(float nuevo_lux)
{
    if (s_total_muestras < VENTANA_MEDIA_MOVIL) {
        // Fase de llenado inicial de la ventana
        s_buffer_muestras[s_indice_escritura] = nuevo_lux;
        s_suma_acumulada += nuevo_lux;
        s_total_muestras++;
        s_indice_escritura = (s_indice_escritura + 1) % VENTANA_MEDIA_MOVIL;
    } else {
        // Ventana completa: restamos el dato antiguo y sumamos el nuevo (FIFO O(1))
        s_suma_acumulada -= s_buffer_muestras[s_indice_escritura];
        s_buffer_muestras[s_indice_escritura] = nuevo_lux;
        s_suma_acumulada += nuevo_lux;
        s_indice_escritura = (s_indice_escritura + 1) % VENTANA_MEDIA_MOVIL;
    }

    return s_suma_acumulada / (float)s_total_muestras;
}

float sensor_adc_obtener_media(void)
{
    if (s_total_muestras == 0) return 0.0f;
    return s_suma_acumulada / (float)s_total_muestras;
}