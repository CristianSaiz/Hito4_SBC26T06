# Hito 4: Telemetría IoT en ThingsBoard (MQTT), Cuadros de Mando y Procesado en el Extremo (Edge Computing)

## Metadatos del Proyecto

* **Asignatura:** Sistemas Basados en Computador (SBC) - ETSISI UPM
* **Identificador de Grupo:** `SBC26T06` (Grupo B)
* **Miembros del Grupo B:**
  * Ana Cuevas
  * Cristian Saiz
* **Plataforma Hardware:** ESP32-WROOM-32 (Xtensa Dual-Core 32-bit LX6)
* **Framework y Entorno:** ESP-IDF v5.x / FreeRTOS Kernel
* **Lenguaje:** C (C99 / C11)
* **Repositorio GitHub:** [Enlace al repositorio](https://github.com/CristianSaiz/Hito4_SBC26T06.git)
* **Espacio de Trabajo SharePoint:** [Directorio del Grupo en SharePoint](https://upm365.sharepoint.com/:f:/s/SBC26T06/IgBiE75sXI7VTaqkI2nbVZ06AcmzC81UPUpKOMSo96Av0NI?e=0CfDkw)

---

## 1. Descripción del Proyecto

Este repositorio contiene la implementación del **Hito 4: Telemetría IoT en ThingsBoard y Cuadros de Mando**, desarrollado sobre el SoC ESP32 en lenguaje C. 

El sistema adquiere periódicamente el nivel de luminosidad ambiente mediante una fotorresistencia (LDR), procesa la señal en local, actualiza interfaces físicas de usuario (display de 7 segmentos y pantalla OLED SSD1306) y publica la información telemétrica formateada en JSON a la plataforma ThingsBoard mediante el protocolo MQTT sobre TCP.

El proyecto está desacoplado en dos arquitecturas independientes:
* **`Hito4_Basico`**: Muestreo analógico, mapeo local en actuadores y telemetría periódica de la variable instantánea (`lux`).
* **`Hito4_Avanzado`**: Procesado digital de señal en el extremo (*Edge Computing*) mediante un buffer circular en firmware que calcula en tiempo real la **media móvil** de los últimos 20 segundos (`lux_avg`), transmitiendo simultáneamente la lectura cruda y la filtrada para su análisis comparativo en el Dashboard.

---

## 2. Estructura del Repositorio

El árbol del proyecto separa estrictamente ambas variantes en proyectos CMake autónomos para facilitar su compilación, auditoría y evaluación independiente:

```text
Hito4_SBC26T06/
├── Hito4_Avanzado/
│   ├── CMakeLists.txt
│   ├── components/
│   │   └── ssd1306/              # Driver de terceros para display OLED I2C
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── actuators.h       # Control de OLED y display 7-segmentos
│   │   │   ├── app_config.h      # Credenciales Wi-Fi y Access Token de ThingsBoard
│   │   │   ├── mqtt_client_app.h # Cliente MQTT (QoS 1 sobre TCP)
│   │   │   ├── sensor_adc.h      # API ADC1 con buffer circular (Media Móvil)
│   │   │   └── wifi_sta.h        # Gestor de conexión Wi-Fi STA con eventos
│   │   ├── Kconfig.projbuild
│   │   ├── main.c                # Orquestador: telemetría dual (lux + lux_avg)
│   │   └── src/
│   │       ├── actuators.c
│   │       ├── mqtt_client_app.c
│   │       ├── sensor_adc.c      # Implementación FIFO O(1) de media móvil (20s)
│   │       └── wifi_sta.c
│   └── sdkconfig
│
├── Hito4_Basico/
│   ├── CMakeLists.txt
│   ├── components/
│   │   └── ssd1306/              # Driver de terceros para display OLED I2C
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── actuators.h
│   │   │   ├── app_config.h
│   │   │   ├── mqtt_client_app.h
│   │   │   ├── sensor_adc.h      # API ADC1 con calibración eFuse Line-Fitting
│   │   │   └── wifi_sta.h
│   │   ├── Kconfig.projbuild
│   │   ├── main.c                # Orquestador: telemetría estándar (lux)
│   │   └── src/
│   │       ├── actuators.c
│   │       ├── mqtt_client_app.c
│   │       ├── sensor_adc.c
│   │       └── wifi_sta.c
│   └── sdkconfig
│
├── .gitignore                    # Exclusión estricta de binarios (build/) y entornos
└── README.md
```

## 3. Asignación de Pines y Hardware (Pinout)

Todas las líneas analógicas se han confinado a la unidad **ADC1** para garantizar compatibilidad con el transceptor Wi-Fi (evitando colisiones con el ADC2).

| Periférico | Pin ESP32 | Tipo / Bus | Descripción / Función |
| :--- | :--- | :--- | :--- |
| **Sensor LDR** | GPIO 34 | Analógico (ADC1_CH6) | Divisor de tensión resistivo (LDR + 10 kΩ pull-down). |
| **OLED SSD1306 (SDA)** | GPIO 21 | Bus I2C (Master) | Línea de datos serie I2C (400 kHz). |
| **OLED SSD1306 (SCL)** | GPIO 22 | Bus I2C (Master) | Línea de reloj serie I2C. |
| **Display 7-Seg (Seg A)** | GPIO 13 | Salida Digital | Decodificación por software (Cátodo Común). |
| **Display 7-Seg (Seg B)** | GPIO 12 | Salida Digital | Línea de segmento B. |
| **Display 7-Seg (Seg C)** | GPIO 14 | Salida Digital | Línea de segmento C. |
| **Display 7-Seg (Seg D)** | GPIO 27 | Salida Digital | Línea de segmento D. |
| **Display 7-Seg (Seg E)** | GPIO 26 | Salida Digital | Línea de segmento E. |
| **Display 7-Seg (Seg F)** | GPIO 25 | Salida Digital | Línea de segmento F. |
| **Display 7-Seg (Seg G)** | GPIO 33 | Salida Digital | Línea de segmento G. |

---

## 4. Arquitectura de Firmware y Edge Computing

### 4.1 Adquisición y Calibración
El sensor analógico opera con el controlador nativo `esp_adc/adc_oneshot.h` a 12 bits de resolución con atenuación de 12 dB (rango de 0 a 3300 mV). Se aplica compensación de no linealidad mediante el esquema *Line Fitting* basado en las curvas de fábrica quemadas en los eFuses del chip (`esp_adc/adc_cali.h`). La tensión compensada se modela numéricamente para obtener la estimación en Lux según las características de la fotorresistencia.

### 4.2 Filtro de Media Móvil (Hito Avanzado)
Para cumplir con los criterios de procesado en el nodo sin depender del cómputo en la nube:
* **Frecuencia de muestreo:** 0.5 Hz (período de muestreo $T_s = 2\text{ s}$).
* **Ventana temporal:** $N = 10\text{ muestras}$ ($10 \times 2\text{ s} = 20\text{ segundos}$).
* **Algoritmo:** Implementado mediante un buffer circular (FIFO) estático con actualización en tiempo constante $O(1)$:

$$\bar{x}_k = \bar{x}_{k-1} + \frac{x_k - x_{k-N}}{N}$$

### 4.3 Pila de Comunicaciones MQTT
* La conexión Wi-Fi STA está sincronizada mediante un `EventGroupHandle_t` de FreeRTOS, garantizando que el socket MQTT no intente conectarse hasta tener concesión DHCP e IP asignada.
* El cliente MQTT se enlaza contra el broker `mqtt://eu.thingsboard.cloud:1883` autenticado mediante el *Device Access Token*.
* Las publicaciones se envían con **QoS 1** sobre el topic estándar `v1/devices/me/telemetry`.

**Estructura del payload JSON transmitido:**
```json
{
  "lux": 225.0,
  "lux_avg": 228.2,
  "voltaje_mv": 2700
}
```

---

## 5. Configuración del Cuadro de Mando (ThingsBoard)

Se han implementado dashboards dedicados con los siguientes elementos de visualización:

1. **Tacómetro Analógico (*Radial Gauge*):** Representa la luminosidad instantánea actual (`lux`) en una escala de 0 a 1200 lx con segmentos diferenciados por color.
2. **Histórico Temporal Multivariable (*Timeseries Chart*):**
   * **Traza Azul (`lux`):** Variación instantánea de la señal. Refleja transitorios rápidos y caídas abruptas al tapar el sensor.
   * **Traza Naranja (`lux_avg`):** Señal procesada por el buffer circular del microcontrolador. Demuestra la inercia de la ventana de 20 segundos y la atenuación del ruido.
   * **Función de agregación:** Desactivada (`None`) para graficar directamente los puntos que publica el ESP32 cada 2 segundos.

---

## 6. Guía de Compilación y Despliegue

### Requisitos previos
* Entorno ESP-IDF configurado (probado en v5.3.1).
* Dispositivo registrado en la instancia de ThingsBoard con su token correspondiente.

### 6.1 Configuración de credenciales
Actualice las constantes de red en el archivo `main/include/app_config.h` del proyecto correspondiente:

```c
#define WIFI_SSID           "TU_RED_WIFI"
#define WIFI_PASS           "TU_PASSWORD"
#define TB_BROKER_URI       "mqtt://eu.thingsboard.cloud:1883"
#define TB_ACCESS_TOKEN     "TU_ACCESS_TOKEN"
```

### 6.2 Compilación y carga de binarios
Desde un terminal con el entorno exportado:

```bash
# Entrar a la carpeta del proyecto a evaluar (por ejemplo, Avanzado)
cd Hito4_Avanzado

# Compilar, flashear y abrir el monitor serie
idf.py -p /dev/ttyUSB0 flash monitor
```

*(Para cerrar el monitor serie: pulsar `Ctrl + ]`)*.

---

## 7. Evidencias y Entrega

* **Vídeo Demostrativo:** Grabación en toma continua mostrando la respuesta coordinada de la fotorresistencia en la protoboard, el display de 7 segmentos, la pantalla OLED y la evolución temporal de ambas trazas en el Dashboard de ThingsBoard.
* **Empaquetado oficial:** Repositorio limpio de binarios generado mediante `idf.py fullclean` para asegurar la exclusión estricta de las carpetas `build/`.