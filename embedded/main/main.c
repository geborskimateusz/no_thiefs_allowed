#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ON_OFF_BUTTON GPIO_NUM_4
#define REED_SWITCH_PIN GPIO_NUM_5
#define LED_ALARM_PIN GPIO_NUM_18
#define LED_OK_PIN GPIO_NUM_19
#define LED_SETUP_PIN GPIO_NUM_21

static const char *TAG = "device";

typedef struct {
    int turnedOn;
    bool lastOnOffState;
    bool setupDeferredDone;
} DeviceState;

DeviceState deviceState = {
    .turnedOn = 0,
    .lastOnOffState = true,  // HIGH
    .setupDeferredDone = false
};

void blink(gpio_num_t pin, int iter, int blinkTimeMs) {
    for (int i = 0; i < iter; i++) {
        gpio_set_level(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(blinkTimeMs));
        gpio_set_level(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(blinkTimeMs));
    }
}

void deferredSetup(DeviceState* state) {
    blink(LED_SETUP_PIN, 3, 1000);

    gpio_set_level(LED_ALARM_PIN, 0);
    gpio_set_level(LED_OK_PIN, 0);
    gpio_set_level(LED_SETUP_PIN, 0);

    state->setupDeferredDone = true;
    ESP_LOGI(TAG, "Deferred setup completed.");
}

void programStateCleanup(DeviceState* state) {
    gpio_set_level(LED_SETUP_PIN, 1);
    gpio_set_level(LED_ALARM_PIN, 0);
    gpio_set_level(LED_OK_PIN, 0);

    state->setupDeferredDone = false;
}

void checkReedSwitchState() {
    int reedSwitchState = gpio_get_level(REED_SWITCH_PIN);

    if (reedSwitchState == 1) {
        ESP_LOGI(TAG, "Circuit is broken or disconnected!");
        gpio_set_level(LED_OK_PIN, 0);
        blink(LED_ALARM_PIN, 5, 300);
        gpio_set_level(LED_ALARM_PIN, 1);
    } else {
        ESP_LOGI(TAG, "Circuit is complete.");
        gpio_set_level(LED_OK_PIN, 1);
        gpio_set_level(LED_ALARM_PIN, 0);
    }
}

void handleOnOffSwitch(DeviceState* state) {
    int onOff = gpio_get_level(ON_OFF_BUTTON);

    if (onOff == 0 && state->lastOnOffState == 1) {
        state->lastOnOffState = 0;
        state->turnedOn = !state->turnedOn;

        if (state->turnedOn && !state->setupDeferredDone) {
            deferredSetup(state);
        }
    } else if (onOff == 1 && state->lastOnOffState == 0) {
        state->lastOnOffState = 1;
    }
}

void app_main(void)
{
    // Configure GPIO pins
    gpio_config_t io_conf = {0};

    // Inputs with pull-up
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    io_conf.pin_bit_mask = (1ULL << ON_OFF_BUTTON) | (1ULL << REED_SWITCH_PIN);
    gpio_config(&io_conf);

    // Outputs
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pin_bit_mask = (1ULL << LED_ALARM_PIN) | (1ULL << LED_OK_PIN) | (1ULL << LED_SETUP_PIN);
    gpio_config(&io_conf);

    gpio_set_level(LED_SETUP_PIN, 1); // Turn LED_SETUP_PIN on

    while (1) {
        handleOnOffSwitch(&deviceState);

        if (deviceState.turnedOn) {
            ESP_LOGI(TAG, "Device is running.");
            checkReedSwitchState();
        } else {
            programStateCleanup(&deviceState);
            ESP_LOGI(TAG, "Device in standby mode.");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
