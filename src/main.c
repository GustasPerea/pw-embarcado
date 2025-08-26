#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "config.h"
#include "flow_sensor.h"
#include "storage.h"

static const char *TAG = "MAIN";

// ===================== BUZZER =====================
void buzzer_init(void)
{
    // Configurar timer PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configurar canal PWM
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_PIN,
        .duty = 0, // Inicia desligado
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

void buzzer_on(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (1 << (LEDC_DUTY_RES - 1))); // 50%
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void buzzer_off(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// ===================== LEDS =====================
static uint8_t led_state = 0;
static int64_t last_led_toggle = 0;

void get_gps_position(char *buffer, size_t len) {
    // placeholder — futuramente integrar SIM7600 NMEA
    snprintf(buffer, len, "-23.5505,-46.6333");
}

void update_led(float flow_Lday) {
    int64_t now = esp_timer_get_time();
    
    if (flow_Lday >= 5000) {
        // Verde
        gpio_set_level(LED_RED_GPIO, 0);
        gpio_set_level(LED_GREEN_GPIO, 1);
        gpio_set_level(LED_BLUE_GPIO, 0);
        buzzer_off();
    } 
    else if (flow_Lday >= 3000) {
        // Azul
        gpio_set_level(LED_RED_GPIO, 0);
        gpio_set_level(LED_GREEN_GPIO, 0);
        gpio_set_level(LED_BLUE_GPIO, 1);
        buzzer_off();
    } 
    else if (flow_Lday >= 1000) {
        // Vermelho fixo
        gpio_set_level(LED_RED_GPIO, 1);
        gpio_set_level(LED_GREEN_GPIO, 0);
        gpio_set_level(LED_BLUE_GPIO, 0);
        buzzer_off();
    } 
    else {
        // Piscar vermelho (500 ms) + buzzer junto
        if (now - last_led_toggle > 500000) {
            led_state = !led_state;
            gpio_set_level(LED_RED_GPIO, led_state);
            gpio_set_level(LED_GREEN_GPIO, 0);
            gpio_set_level(LED_BLUE_GPIO, 0);

            if (led_state) {
                buzzer_on();
            } else {
                buzzer_off();
            }

            last_led_toggle = now;
        }
    }
}

// ===================== MAIN =====================
void app_main(void) {
    // serial
    esp_log_level_set("*", ESP_LOG_INFO);
    printf("\n\n%s booting...\n", DEVICE_NAME);

    storage_init();
    flow_sensor_init();
    buzzer_init();   // <-- inicializa o buzzer

    // Botão de reset
    gpio_config_t btn_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&btn_conf);

    // LEDs
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

        // Botão reset
        if (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
            storage_reset_hidrometer();
            totalLiters = 0.0f;
            ESP_LOGW(TAG, "Botão pressionado -> Reset do hidrômetro");
            vTaskDelay(pdMS_TO_TICKS(500)); // debounce
        }

        int64_t now = esp_timer_get_time();
        float interval = (now - last_time) / 1000000.0f;
        if (interval <= 0.0f) interval = 1.0f;
        last_time = now;

        float increment = flow_sensor_get_increment(interval);
        totalLiters += increment;
        storage_save_hidrometer(totalLiters);

        float flow_Lmin = (increment / interval) * 60.0f;
        float flow_Lday = flow_Lmin * 60.0f * 24.0f;

        update_led(flow_Lday);

        // Local time
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
