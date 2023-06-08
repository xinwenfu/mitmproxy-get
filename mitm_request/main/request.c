// C Standard input output lib and string libs
#include <stdio.h>
#include <string.h>
// ESP and RTOS libs
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
// Wi-Fi library
#include <esp_wifi.h>
// HTTP Library
#include <esp_http_client.h>
// TLS Library
#include <esp_tls.h>
// Lightweight TCP/IP libraries 
#include "lwip/err.h"
#include "lwip/sys.h"
// Sensor Libraries 
#include <aht.h>

// Could not find where they define MIN
#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

// Tag used in logs generated
char* TAG = "HTTP_MITM_PROXY";

// HTTP/HTTPS defines to change behavior
#define FU_HTTP
// #define FU_HTTPS

// AHT Defines 
#define SDA_GPIO 27
#define SCL_GPIO 33
#define AHT_TYPE AHT_TYPE_AHT1x

// Define the max length of a URL (Number of chars)
#define MAX_URL_LEN 480

// Change ubuntu-vm-ip below to your Ubuntu VM IP
char* serverName = "ubuntu-vm-ip/test_get.php";


// Specify Server Cert file (loaded as binary)
// Do not change unless the filename is changed!
extern const char server_cert_crt_start[] asm("_binary_server_cert_crt_start");
extern const char server_cert_crt_end[]   asm("_binary_server_cert_crt_end");

// WIFI Defines
// These are from the station example reference linked to at the wifi function comment
/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
// END WIFI DEFINES

// HTTP Defines
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
// END HTTP Defines

// Function Forward definitions
int aht_read(float* temperature, float* humidity);
void wifi_init_sta(void);
void http_request(void*);

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

//********************************************************
// Function: Main function, entrypoint to the application
//Arguments: None
//Return Value: None
//********************************************************
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // IC2 Init
    ESP_ERROR_CHECK(i2cdev_init());
    // Connect to WiFi
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA Start");
    wifi_init_sta();
    // Create HTTP Task
    xTaskCreate(&http_request, "http_request_task", 8192, NULL, 1, NULL);
}

//********************************************************
// Function: This is an event handler for HTTP requests, simply writes errors to the data log
// Arguments: HTTP client event value
// Ref: https://github.com/espressif/esp-idf/blob/master/examples/protocols/esp_http_client/main/esp_http_client_example.c
//********************************************************
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  static char *output_buffer;  // Buffer to store response of http request from event handler
  static int output_len;       // Stores number of bytes read
  switch(evt->event_id) {
      case HTTP_EVENT_ERROR:
          ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
          break;
      case HTTP_EVENT_ON_CONNECTED:
          ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
          break;
      case HTTP_EVENT_HEADER_SENT:
          ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
          break;
      case HTTP_EVENT_ON_HEADER:
          ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
          break;
      case HTTP_EVENT_ON_DATA:
          ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
          /*
           *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
           *  However, event handler can also be used in case chunked encoding is used.
           */
          if (!esp_http_client_is_chunked_response(evt->client)) {
              // If user_data buffer is configured, copy the response into the buffer
              int copy_len = 0;
              if (evt->user_data) {
                  copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                  if (copy_len) {
                      memcpy(evt->user_data + output_len, evt->data, copy_len);
                  }
              } else {
                  const int buffer_len = esp_http_client_get_content_length(evt->client);
                  if (output_buffer == NULL) {
                      output_buffer = (char *) malloc(buffer_len);
                      output_len = 0;
                      if (output_buffer == NULL) {
                          ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                          return ESP_FAIL;
                      }
                  }
                  copy_len = MIN(evt->data_len, (buffer_len - output_len));
                  if (copy_len) {
                      memcpy(output_buffer + output_len, evt->data, copy_len);
                  }
              }
              output_len += copy_len;
          }
          break;
      case HTTP_EVENT_ON_FINISH:
          ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
          if (output_buffer != NULL) {
              // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
              // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
              free(output_buffer);
              output_buffer = NULL;
          }
          output_len = 0;
          break;
      case HTTP_EVENT_DISCONNECTED:
          ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
          int mbedtls_err = 0;
          esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
          if (err != 0) {
              ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
              ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
          }
          if (output_buffer != NULL) {
              free(output_buffer);
              output_buffer = NULL;
          }
          output_len = 0;
          break;
/* This does not work (exist) in esp-idf version 4.4
      case HTTP_EVENT_REDIRECT:
          ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
          esp_http_client_set_header(evt->client, "From", "user@example.com");
          esp_http_client_set_header(evt->client, "Accept", "text/html");
          esp_http_client_set_redirection(evt->client);
          break;
*/
  }
  return ESP_OK;
}


//********************************************************
// Function: This is a function that will make the HTTP or 
// HTTPS request using the ESP32 HTTP client library
//
// Arguments: Generic pointer (required by xTaskCreate)
// Return Value: None
// Ref: For the HTTP client functions https://github.com/espressif/esp-idf/blob/master/examples/protocols/esp_http_client/main/esp_http_client_example.c#L763 
//********************************************************

void http_request(void* args) {
  float temp = 0.0, humidity = 0.0;
  char url_str[MAX_URL_LEN];
  char local_response_buffer[MAX_HTTP_RECV_BUFFER] = {0};

  while(1) {
    // Delay Some number of tics to prevent overheating 
    // if 1 tick == 1 ms then this is every 30seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    // Read AHT data, on failure skip iteration
    if (!aht_read(&temp,&humidity)){
      ESP_LOGE(TAG, "Error when reading AHT data");
      continue;
    }
    // Generate URL using the Temp and Humidity values
    #ifdef FU_HTTP
    sprintf(url_str, "http://%s?Temperature=%.2f&Humidity=%.2f",serverName,temp,humidity);
    # endif
    #ifdef FU_HTTPS
    sprintf(url_str, "https://%s?Temperature=%.2f&Humidity=%.2f",serverName,temp,humidity);
    #endif
    ESP_LOGI(TAG, "URL: %s", url_str);

    // HTTP options are defined at 
    esp_http_client_config_t config = {
        
        .url = url_str,
        .user_data = local_response_buffer,
        #ifdef FU_HTTPS
        .cert_pem = server_cert_crt_start,
        #endif
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };
    // Init HTTP Client
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Perform GET Request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Server Response:\n%s", local_response_buffer);
    
    // Cleanup Client
    esp_http_client_cleanup(client);
  }
}



//********************************************************
// Function: This is a function called on the completion of an event
// This is used for the WIFI functions
// Arguments: argument list, event, event_id and data (generic)
// Return Value: None
// Ref: https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station
//********************************************************
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static int s_retry_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


//********************************************************
// Function: Initialize WiFi, blocking operation
// Arguments: None
// Return Value: None
//
// Ref: https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station
//
// NOTE: We can use a helper function instep of this as referenced 
// https://github.com/espressif/esp-idf/tree/master/examples/protocols
//********************************************************
void wifi_init_sta(void)
{
    
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
  
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            //.sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}


//********************************************************
// Function: Utilize ESP32 AHT Library to load values into 
// The pontes float arguments 
//
// Arguments:
// float* temperature: Pointer to float, temp value read from 
// the AHT will be stored here after return (If return of func is 1)
//
// float* humidity: Pointer to humidity value read from the AHT
// will be stored here after return (IF return of func is 1)
//
// Return Value:
// 1: Function successfully read Temp and Humidity values
// 0: otherwise
// 
// Ref: https://github.com/UncleRus/esp-idf-lib/tree/master/examples/aht/default
//********************************************************
int aht_read(float* temperature, float* humidity) {
  aht_t dev = { 0 };
  dev.mode = AHT_MODE_NORMAL;
  dev.type = AHT_TYPE;

  // Initalize AHT Device
  ESP_ERROR_CHECK(aht_init_desc(&dev, AHT_I2C_ADDRESS_GND, (i2c_port_t)0, SDA_GPIO, SCL_GPIO));
  ESP_ERROR_CHECK(aht_init(&dev));

  // Calibrate AHT Device
  bool calibrated;
  ESP_ERROR_CHECK(aht_get_status(&dev, NULL, &calibrated));
  if (calibrated)
      ESP_LOGI(TAG, "Sensor calibrated");
  else
      ESP_LOGW(TAG, "Sensor not calibrated!");
  
  // Read from device, write the values to pointers passed to the function.
  // Also Print values here.
  esp_err_t res = aht_get_data(&dev, temperature, humidity);
  if (res != ESP_OK) {
     ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));     vTaskDelay(pdMS_TO_TICKS(500));
     return 0;
  }
    
  ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.2f%%", *temperature, *humidity);
  return 1;  
}