#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "config.h"
#include "flow_sensor.h"
#include "storage.h"

static const char *TAG = "MAIN";

// Variáveis para controle do LED
static uint8_t led_state = 0;
static int64_t last_led_toggle = 0;

void get_gps_position(char *buffer, size_t len) {
    // placeholder — futuramente integrar SIM7600 NMEA
    snprintf(buffer, len, "-23.5505,-46.6333");
}

void update_led(float flow_Lday) {
    int64_t now = esp_timer_get_time();
    
if (flow_Lday >= 5000) {
        // Verde (desliga vermelho e azul, liga verde)
        gpio_set_level(LED_RED_GPIO, 0);
        gpio_set_level(LED_GREEN_GPIO, 1);
        gpio_set_level(LED_BLUE_GPIO, 0);
    } 
    else if (flow_Lday >= 3000) {
        // Azul (desliga vermelho e verde, liga azul)
        gpio_set_level(LED_RED_GPIO, 0);
        gpio_set_level(LED_GREEN_GPIO, 0);
        gpio_set_level(LED_BLUE_GPIO, 1);
    } 
    else if (flow_Lday >= 1000) {
        // Vermelho
        gpio_set_level(LED_RED_GPIO, 1);
        gpio_set_level(LED_GREEN_GPIO, 0);
        gpio_set_level(LED_BLUE_GPIO, 0);
    } 
    else {
        // Piscar vermelho (500ms intervalo)
        if (now - last_led_toggle > 500000) {
            led_state = !led_state;
            gpio_set_level(LED_RED_GPIO, led_state);
            gpio_set_level(LED_GREEN_GPIO, 0);
            gpio_set_level(LED_BLUE_GPIO, 0);
            last_led_toggle = now;
        }
    }
}

void app_main(void) {
    // serial
    esp_log_level_set("*", ESP_LOG_INFO);
    printf("\n\n%s booting...\n", DEVICE_NAME);

    storage_init();
    flow_sensor_init();

    // configure reset button (active LOW)
    gpio_config_t btn_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&btn_conf);

    // Configurar os pinos do LED como saídas
    gpio_config_t led_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_RED_GPIO) | (1ULL << LED_GREEN_GPIO) | (1ULL << LED_BLUE_GPIO),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&led_conf);

    float totalLiters = storage_load_hidrometer();
    int64_t last_time = esp_timer_get_time();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s loop

        // check reset button (active low)
        if (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
            storage_reset_hidrometer();
            totalLiters = 0.0f;
            ESP_LOGW(TAG, "Botão pressionado -> Reset do hidrômetro");
            // simple debounce
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        int64_t now = esp_timer_get_time();
        float interval = (now - last_time) / 1000000.0f;
        if (interval <= 0.0f) interval = 1.0f;
        last_time = now;

        float increment = flow_sensor_get_increment(interval);
        totalLiters += increment;
        storage_save_hidrometer(totalLiters);

        float flow_Lmin = (increment / interval) * 60.0f; // L/min
        float flow_Lday = flow_Lmin * 60.0f * 24.0f;     // L/day

        // Atualizar LED baseado na vazão
        update_led(flow_Lday);

        // local time (may be epoch 1970 until RTC/NTP configured)
        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);

        char gps[32];
        get_gps_position(gps, sizeof(gps));

        printf("%s %04d-%02d-%02d %02d:%02d:%02d | Hidrometro: %.3f L | Vazao Estimada: %.2f L/dia | GPS: %s\n",
               DEVICE_NAME,
               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
               tm.tm_hour, tm.tm_min, tm.tm_sec,
               totalLiters, flow_Lday, gps);
    }
}