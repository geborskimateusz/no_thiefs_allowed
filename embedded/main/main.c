#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_http_client.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"

static const char* TAG = "AlarmModule";

#define CODE_OK                 0
#define CODE_REED_SWITCH_BROKEN 1

// LED MODULE
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


// WIFI MODULE
#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;


// Wi-Fi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected, got IP");
    }
}

// Initialize Wi-Fi STA mode
static void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    // Only call global network init once
    static bool netif_initialized = false;
    if (!netif_initialized) {
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        netif_initialized = true;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));

    ESP_LOGI(TAG, "Connecting to SSID:%s", CONFIG_WIFI_SSID);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// HTTP POST task
static void http_post_task(void *pvParameters)
{
    // Wait for Wi-Fi
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    uint32_t reed_switch_status_code = (uint32_t) pvParameters;
    char post_data[128];  
    snprintf(post_data, sizeof(post_data),"{\"message\": %ld}", reed_switch_status_code);
    ESP_LOGI(TAG, "CALLING /api/health with body: %s", post_data);


    char web_server_url[128];
    snprintf(web_server_url, sizeof(web_server_url), "%s/api/health", CONFIG_WEB_SERVER_URL);

    char web_server_token[128];
    snprintf(web_server_token, sizeof(web_server_token), "Basic %s", CONFIG_WEB_SERVER_TOKEN);
    
    esp_http_client_config_t config = {
        .url = web_server_url,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_TCP, 
        .timeout_ms = 10000,  
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", web_server_token);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d", status);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}


// MAIN APP
void app_main(void) {
    wifi_init(); // Start Wi-Fi

    // Wait until connected before starting main loop
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Network ready, starting main loop.");

    configure_led_pins();
    set_device_ready_state();
    configure_reed_switch();

    while (1) {
        int reed_switch_pin_level = gpio_get_level(REED_SWITCH_PIN);
        uint32_t status_code = (reed_switch_pin_level == 0) ? CODE_OK : CODE_REED_SWITCH_BROKEN;

        if (reed_switch_pin_level == 0) {
            set_ok_state();
        } else {
            set_alarm_state();
        }


        http_post_task((void *)status_code);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}