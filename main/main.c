#include "globals.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_system.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"


static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static const char* TAG = "Main";


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, RELAY_CHANNEL, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            RELAY_CONNECTED = atoi(event->data) ? 0: 1  ;
            RELAY_CHANGED = true;
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_ESP_MQTT_URI,
        .event_handle = mqtt_event_handler,
        .username = CONFIG_ESP_MQTT_USERNAME,
        .password = CONFIG_ESP_MQTT_PASSWORD,
        .port = 1883,
        // .user_context = (void *)your_context
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}



static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "STARTED");
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "WIFI CONNECTED");
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    mqtt_app_start();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "WIFI DISCONNECTED!!");
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void wifi_init(void)
{
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
    .sta = {
       .ssid = CONFIG_ESP_WIFI_SSID,
       .password = CONFIG_ESP_WIFI_PASSWORD,
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", "WIFI SSID", "******");
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Waiting for wifi");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}


void app_main(void)
{
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_PIN, 1);
    nvs_flash_erase();
    nvs_flash_init();
    wifi_init();

    printf("Ready...\n");
    
    while(true) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if (RELAY_CHANGED) {
            RELAY_CHANGED = false;
            gpio_set_level(RELAY_PIN, RELAY_CONNECTED);
        }
    }


}
