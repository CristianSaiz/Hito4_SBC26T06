#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ============================================================================
// CONFIGURACIÓN WI-FI STA
// ============================================================================
#define WIFI_SSID           "TU_RED_WIFI"
#define WIFI_PASS           "TU_PASSWORD"

// ============================================================================
// CONFIGURACIÓN THINGSBOARD MQTT
// ============================================================================
// OPCIONES DE BROKER MQTT PARA THINGSBOARD (Descomenta la que corresponda)
// ============================================================================

// 1. ThingsBoard Cloud (Instancia global de ThingsBoard Cloud)
// #define TB_BROKER_URI       "mqtt://thingsboard.cloud:1883"

// 2. ThingsBoard Cloud Europa (Si en la URL de tu navegador pone eu.thingsboard.cloud)
#define TB_BROKER_URI    "mqtt://eu.thingsboard.cloud:1883"

// 3. Servidor Demo Clásico (Instancia comunitaria abierta demo.thingsboard.io)
// #define TB_BROKER_URI    "mqtt://demo.thingsboard.io:1883"

// 4. Servidor Local / Servidor de Laboratorio UPM (Si los profesores usan una máquina local en el aula)
// #define TB_BROKER_URI    "mqtt://192.168.1.X:1883"

#define TB_ACCESS_TOKEN     "5JLdU8oKDaJxdsFyAdno" // Copiado de tu dispositivo TB
#define TB_TELEMETRY_TOPIC  "v1/devices/me/telemetry"

#endif // APP_CONFIG_H