#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "esp_log.h"
#include "driver/gpio.h"

static const char* TAG = "AlarmModule";

static const int YELLOW_LED_PIN = 21;
static const int GREEN_LED_PIN = 19;
static const int RED_LED_PIN = 18;
static const int REED_SWITCH_PIN = 5;

void set_device_ready_state() {
    gpio_set_level(YELLOW_LED_PIN, 1);
    gpio_set_level(GREEN_LED_PIN, 0);
    gpio_set_level(RED_LED_PIN, 0);
    ESP_LOGI(TAG, "Device is running, status: ready to use.");
}

void set_ok_state() {
    gpio_set_level(YELLOW_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 1);
    gpio_set_level(RED_LED_PIN, 0);
    ESP_LOGI(TAG, "Device is running, status: reed switch circuit is ok.");
}

void set_alarm_state() {
    gpio_set_level(YELLOW_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 0);
    gpio_set_level(RED_LED_PIN, 1);
    ESP_LOGI(TAG, "Device is running, status: reed switch circuit is broken!");
}
    
void configure_led_pins() {
    ESP_LOGI(TAG, "Configuring LED GPIOs.");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << YELLOW_LED_PIN) | (1ULL << GREEN_LED_PIN) | (1ULL << RED_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LED GPIOs configured.");
}

void configure_reed_switch() {
    ESP_LOGI(TAG, "Configuring reed switch GPIOs.");

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << REED_SWITCH_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Reed switch GPIOs configured.");
}

void app_main(void) {
    configure_led_pins();
    set_device_ready_state();
    configure_reed_switch();

    while (1) {
        int reed_switch_pin_level = gpio_get_level(REED_SWITCH_PIN);
        if (reed_switch_pin_level == 0) {
            set_ok_state();
        } else {
            set_alarm_state();
        }
        sleep(1);
    }
}