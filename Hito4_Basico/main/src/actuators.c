#include "actuators.h"
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ssd1306.h"

static const char *TAG = "ACTUADORES";

// -------------------------------------------------------------
// Pines para Display 7 Segmentos (Anodo Comun)
// -------------------------------------------------------------
#define PIN_SEG_A   GPIO_NUM_13
#define PIN_SEG_B   GPIO_NUM_12
#define PIN_SEG_C   GPIO_NUM_14
#define PIN_SEG_D   GPIO_NUM_27
#define PIN_SEG_E   GPIO_NUM_26
#define PIN_SEG_F   GPIO_NUM_25
#define PIN_SEG_G   GPIO_NUM_33

static const gpio_num_t pines_segmentos[7] = {
    PIN_SEG_A, PIN_SEG_B, PIN_SEG_C, PIN_SEG_D, 
    PIN_SEG_E, PIN_SEG_F, PIN_SEG_G
};

#define MASCARA_7SEG ( (1ULL << PIN_SEG_A) | (1ULL << PIN_SEG_B) | \
                       (1ULL << PIN_SEG_C) | (1ULL << PIN_SEG_D) | \
                       (1ULL << PIN_SEG_E) | (1ULL << PIN_SEG_F) | \
                       (1ULL << PIN_SEG_G) )

static const uint8_t digitos_7seg[10] = {
    0b01000000, // 0
    0b01111001, // 1
    0b00100100, // 2
    0b00110000, // 3
    0b00011001, // 4
    0b00010010, // 5
    0b00000010, // 6
    0b01111000, // 7
    0b00000000, // 8
    0b00010000  // 9
};

// Pines del bus I2C para pantalla OLED
#define OLED_PIN_SDA    GPIO_NUM_21
#define OLED_PIN_SCL    GPIO_NUM_22

static SSD1306_t pantalla_oled;
static bool oled_ok = false;

// Buffer circular para la grafica temporal historica (Timeseries)
#define NUM_PUNTOS_GRAFICA 38
static float buffer_historico[NUM_PUNTOS_GRAFICA];
static bool historico_inicializado = false;

esp_err_t actuadores_init(void)
{
    // 1. Inicializar pines del 7 segmentos
    gpio_config_t config_gpio = {
        .pin_bit_mask = MASCARA_7SEG,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config_gpio);

    for (int i = 0; i < 7; i++) {
        gpio_set_level(pines_segmentos[i], 1);
    }

    // 2. Inicializar OLED I2C
    i2c_master_init(&pantalla_oled, OLED_PIN_SDA, OLED_PIN_SCL, -1);
    ssd1306_init(&pantalla_oled, 128, 64);
    ssd1306_clear_screen(&pantalla_oled, false);
    ssd1306_contrast(&pantalla_oled, 0xFF);
    oled_ok = true;

    // Mensaje de arranque
    ssd1306_display_text(&pantalla_oled, 0, "  HITO 4 - UPM  ", 16, false);
    ssd1306_display_text(&pantalla_oled, 2, "Conectando WiFi ", 16, false);
    ssd1306_display_text(&pantalla_oled, 4, "ThingsBoard...  ", 16, false);

    ESP_LOGI(TAG, "Actuadores configurados con soporte grafico");
    return ESP_OK;
}

void actuadores_mostrar_nivel_7seg(int nivel)
{
    if (nivel < 0) nivel = 0;
    if (nivel > 9) nivel = 9;

    uint8_t patron = digitos_7seg[nivel];
    for (int i = 0; i < 7; i++) {
        int bit_val = (patron >> i) & 0x01;
        gpio_set_level(pines_segmentos[i], bit_val);
    }
}

void actuadores_actualizar_oled(float lux, int mv, bool conectado_mqtt)
{
    if (!oled_ok) return;

    // Inicializar el historico con el primer dato leido
    if (!historico_inicializado) {
        for (int i = 0; i < NUM_PUNTOS_GRAFICA; i++) {
            buffer_historico[i] = lux;
        }
        historico_inicializado = true;
    } else {
        // Desplazar muestras a la izquierda para el scroll
        for (int i = 0; i < NUM_PUNTOS_GRAFICA - 1; i++) {
            buffer_historico[i] = buffer_historico[i + 1];
        }
        buffer_historico[NUM_PUNTOS_GRAFICA - 1] = lux;
    }

    char linea[32];

    // --- LINEA 1: Estado del broker MQTT (16 caracteres) ---
    if (conectado_mqtt) {
        ssd1306_display_text(&pantalla_oled, 0, "TB: ONLINE MQTT", 16, false);
    } else {
        ssd1306_display_text(&pantalla_oled, 0, "TB: CONECTANDO.. ", 16, false);
    }

    // --- LINEA 2: "Luz: [barra] %" (16 caracteres exactos) ---
    int porcentaje = (int)((lux / 1000.0f) * 100.0f);
    if (porcentaje < 0) porcentaje = 0;
    if (porcentaje > 100) porcentaje = 100;

    // 5 bloques: cada '#' representa un 20%
    int bloques = (porcentaje + 10) / 20;
    if (bloques > 5) bloques = 5;

    char barra[6];
    for (int i = 0; i < 5; i++) {
        barra[i] = (i < bloques) ? '#' : ' ';
    }
    barra[5] = '\0';

    // Monta: "Luz: [" (6) + barra (5) + "]" (1) + " 17%" (4) = 16 caracteres
    snprintf(linea, sizeof(linea), "Luz: [%s]%3d%%", barra, porcentaje);
    ssd1306_display_text(&pantalla_oled, 1, linea, 16, false);

    // --- PARTE INFERIOR: GRAFICA TEMPORAL HISTORICA ---
    for (int pag = 2; pag < 8; pag++) {
        memset(pantalla_oled._page[pag]._segs, 0x00, 128);
    }

    // Ejes y marco exterior
    _ssd1306_line(&pantalla_oled, 0, 18, 127, 18, false);   // Marco superior
    _ssd1306_line(&pantalla_oled, 6, 20, 6, 63, false);     // Eje Y izquierdo
    _ssd1306_line(&pantalla_oled, 6, 63, 124, 63, false);   // Eje X suelo
    _ssd1306_line(&pantalla_oled, 124, 20, 124, 63, false); // Eje vertical derecho

    // Unir las muestras con lineas continuas
    const int x_inicio = 8;
    const int paso_x = 3;

    for (int i = 0; i < NUM_PUNTOS_GRAFICA - 1; i++) {
        int x1 = x_inicio + (i * paso_x);
        int x2 = x_inicio + ((i + 1) * paso_x);

        int y1 = 62 - (int)((buffer_historico[i] / 1000.0f) * 40.0f);
        if (y1 < 22) y1 = 22;
        if (y1 > 62) y1 = 62;

        int y2 = 62 - (int)((buffer_historico[i + 1] / 1000.0f) * 40.0f);
        if (y2 < 22) y2 = 22;
        if (y2 > 62) y2 = 62;

        _ssd1306_line(&pantalla_oled, x1, y1, x2, y2, false);
    }

    ssd1306_show_buffer(&pantalla_oled);
}