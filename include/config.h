#pragma once

#define DEVICE_NAME        "Central"
#define FLOW_SENSOR_GPIO   25
#define RESET_BUTTON_GPIO  12
#define FLOW_K_FACTOR      7.5f   // K: frequency(Hz) = flow(L/min) * K
#define SERIAL_BAUD        115200
// config.h
#ifndef CONFIG_H
#define CONFIG_H

// Definições dos pinos do LED RGB (ânodo comum)
#define LED_RED_GPIO       26
#define LED_GREEN_GPIO     27
#define LED_BLUE_GPIO   33



// Defina o pino do buzzer (ajuste conforme sua conexão)
#define BUZZER_PIN GPIO_NUM_18

// Configuração do PWM para o buzzer
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT // Resolução de 13 bits
#define LEDC_FREQUENCY      4000              // Frequência em Hz

#endif