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
#define LED_BLUE_GPIO      33

#endif