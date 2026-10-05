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

esp_err_t sensor_adc_init(void)
{
    // Configuramos la unidad ADC1 en modo oneshot
    adc_oneshot_unit_init_cfg_t config_unidad = {
        .unit_id = UNIDAD_ADC,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&config_unidad, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear la unidad ADC: %s", esp_err_to_name(err));
        return err;
    }

    // Canal 6 a 12 bits de resolucion
    adc_oneshot_chan_cfg_t config_canal = {
        .atten = ATENUACION_LDR,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CANAL_LDR, &config_canal));

    // Intentamos cargar la calibracion de fabrica por eFuse
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t config_cali = {
        .unit_id = UNIDAD_ADC,
        .atten = ATENUACION_LDR,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&config_cali, &cali_handle) == ESP_OK) {
        calibrado = true;
        ESP_LOGI(TAG, "Calibracion eFuse cargada");
    } else {
        ESP_LOGW(TAG, "Sin calibracion eFuse, se usara aproximacion lineal");
    }
#endif

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
            // Regla de tres directa a 3.3V si no hay eFuse
            *mv_out = (lectura_raw * 3300) / 4095;
        }
    }

    return ESP_OK;
}

float sensor_adc_calcular_lux(int mv)
{
    // Evitamos divisiones raras o voltajes negativos
    if (mv < 50) mv = 50;
    if (mv > 3250) mv = 3250;

    // Calculo aproximado segun el divisor resistivo con R=10k
    // A mas luz, baja la resistencia de la LDR y sube el voltaje en la pull-down
    float v_out = (float)mv / 1000.0f;
    float r_ldr = (3.3f - v_out) * 10000.0f / v_out;

    // Aproximacion habitual para LDR estandar GL5528
    float lux = 500.0f / (r_ldr / 1000.0f);
    if (lux < 0.0f) lux = 0.0f;
    if (lux > 1500.0f) lux = 1500.0f;

    return lux;
}