#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <utils.h>
#include <usb_serial_io.h>

static const char *TAG = "WIFI_APP";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;
static int s_retry_num = 0;
static char s_ip_addr[16] = {0};

// Auth mode to string
static const char* get_auth_mode_name(wifi_auth_mode_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
        case WIFI_AUTH_WAPI_PSK: return "WAPI-PSK";
        default: return "Unknown";
    }
}

static int compare_rssi(const void *a, const void *b) {
    return ((wifi_ap_record_t *)b)->rssi - ((wifi_ap_record_t *)a)->rssi;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // We will connect manually after selection
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_addr, sizeof(s_ip_addr), IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Block and wait for a single line of input over usbio
static void wait_for_input(char *buf, size_t max_len) {
    size_t pos = 0;
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    if (pos > 0) { // ignore empty line
                        buf[pos] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return; // Done
                    }
                } else if (c == '\b' || c == 127) { // Handle backspace
                    if (pos > 0) {
                        pos--;
                        usbio_print(10, "\b \b"); // attempt to clear char on terminal
                    }
                } else {
                    if (pos < max_len - 1) {
                        buf[pos++] = c;
                        // Echo the character
                        char echo[2] = {c, '\0'};
                        usbio_print(10, echo);
                    }
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        delay_ms(10);
    }
}

void wifi_connect_run(void) {
    char pbuf[256];
    
    // 1. Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    wifi_event_group = xEventGroupCreate();
    s_retry_num = 0;

    static bool is_wifi_init = false;
    if (!is_wifi_init) {
        // Init netif only once
        esp_netif_init(); // safe to call multiple times if already running
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        
        // Init WiFi
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        
        is_wifi_init = true;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // 2. Scan
    usbio_print(100, "Scanning for WiFi networks...\r\n");
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };
    esp_wifi_scan_start(&scan_config, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_records != NULL) {
        esp_wifi_scan_get_ap_records(&ap_count, ap_records);

        // Sort descending by RSSI
        qsort(ap_records, ap_count, sizeof(wifi_ap_record_t), compare_rssi);

        // Filter duplicates by SSID (keep the one with highest RSSI since it's sorted)
        uint16_t unique_count = 0;
        for (int i = 0; i < ap_count; i++) {
            bool duplicate = false;
            for (int j = 0; j < unique_count; j++) {
                if (strcmp((char*)ap_records[i].ssid, (char*)ap_records[j].ssid) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                if (i != unique_count) {
                    ap_records[unique_count] = ap_records[i];
                }
                unique_count++;
            }
        }
        ap_count = unique_count;

        int print_count = (ap_count < 20) ? ap_count : 20;
        snprintf(pbuf, sizeof(pbuf), "Found %d unique access points. Top %d:\r\n", ap_count, print_count);
        usbio_print(100, pbuf);

        for (int i = 0; i < print_count; i++) {
            snprintf(pbuf, sizeof(pbuf), " [%2d] SSID: %-32s | RSSI: %-4d | Auth: %s\r\n", 
                     i + 1, ap_records[i].ssid, ap_records[i].rssi, get_auth_mode_name(ap_records[i].authmode));
            usbio_print(100, pbuf);
        }
    }

    // 3. Select SSID
    usbio_print(100, "Enter SSID (name or number) to connect: ");
    char input_ssid[64] = {0};
    wait_for_input(input_ssid, sizeof(input_ssid));

    // Support numbered selection
    bool is_number = true;
    size_t in_len = strlen(input_ssid);
    for (int i = 0; i < in_len; i++) {
        if (input_ssid[i] < '0' || input_ssid[i] > '9') {
            is_number = false;
            break;
        }
    }

    if (is_number && in_len > 0 && ap_records != NULL) {
        int idx = atoi(input_ssid);
        if (idx >= 1 && idx <= ap_count) {
            strncpy(input_ssid, (char*)ap_records[idx - 1].ssid, sizeof(input_ssid) - 1);
            input_ssid[sizeof(input_ssid) - 1] = '\0';
        }
    }
    
    snprintf(pbuf, sizeof(pbuf), "\r\nSelected SSID: %s\r\n", input_ssid);
    usbio_print(100, pbuf);

    // Find auth mode
    wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK; // default
    bool found = false;
    if (ap_records != NULL) {
        for (int i = 0; i < ap_count; i++) {
            if (strcmp((char*)ap_records[i].ssid, input_ssid) == 0) {
                auth_mode = ap_records[i].authmode;
                found = true;
                break;
            }
        }
        free(ap_records);
        ap_records = NULL; // Prevent double free
    }

    // 4. Ask for Password if needed
    char input_password[64] = {0};
    if (!found || auth_mode == WIFI_AUTH_OPEN) {
        if (!found) {
            usbio_print(100, "Network not in list, assuming WPA2-PSK. Enter Password: ");
            wait_for_input(input_password, sizeof(input_password));
            usbio_print(100, "\r\n");
        } else {
            usbio_print(100, "Open Network Selected (No Password required).\r\n");
        }
    } else {
        usbio_print(100, "Enter Password: ");
        wait_for_input(input_password, sizeof(input_password));
        usbio_print(100, "\r\n");
    }

    // 5. Connect
    snprintf(pbuf, sizeof(pbuf), "Connecting to SSID: '%s' with Password: '%s'...\r\n", input_ssid, input_password);
    usbio_print(100, pbuf);
    
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, input_ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (strlen(input_password) > 0) {
        strncpy((char*)wifi_config.sta.password, input_password, sizeof(wifi_config.sta.password) - 1);
    }
    
    // Set proper auth config
    wifi_config.sta.threshold.authmode = auth_mode;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    // Wait for IP
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        snprintf(pbuf, sizeof(pbuf), "Connected! IP Address: %s\r\n", s_ip_addr);
        usbio_print(100, pbuf);
        
        // Print MAC
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(pbuf, sizeof(pbuf), "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        usbio_print(100, pbuf);
    } else {
        usbio_print(100, "Failed to connect to WiFi.\r\n");
    }

    // Clean up
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(wifi_event_group);

    usbio_print(100, "Done. Returning to menu...\r\n");
}
