#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <utils.h>
#include <usb_serial_io.h>

static void trim_whitespace(char *str) {
    if (!str || !*str) return;
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    int len = strlen(str);
    while (len > 0) {
        char c = str[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            str[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }
}

static void wait_for_input_ns(char *buf, size_t max_len) {
    size_t pos = 0;
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    if (pos > 0) {
                        buf[pos] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    } else if (c == '\r') {
                        // Accept \r as empty enter, ignore bare \n if pos == 0 to avoid trailing \n bugs
                        buf[0] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    }
                } else if (c == '\b' || c == 127) {
                    if (pos > 0) {
                        pos--;
                        usbio_print(10, "\b \b");
                    }
                } else if (c >= 32 && c <= 126) {
                    if (pos < max_len - 1) {
                        buf[pos++] = c;
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

static void wait_for_enter(void) {
    usbio_print(100, "Press enter to continue...\r\n");
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    usbio_rx_data_destruct(&rx);
                    return;
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        delay_ms(10);
    }
}

void network_status_run(void) {
    char pbuf[256];
    
    usbio_print(100, "========================================\r\n");
    usbio_print(100, "            Network Status              \r\n");
    usbio_print(100, "========================================\r\n");

    wifi_ap_record_t ap_info;
    bool connected = false;
    
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        connected = true;
    }

    uint8_t mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK) {
        // Fallback to default base MAC if WiFi not initialized
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
    }
    snprintf(pbuf, sizeof(pbuf), "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    usbio_print(100, pbuf);

    if (connected) {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif) {
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(netif, &ip_info);
            
            snprintf(pbuf, sizeof(pbuf), "Status:      Connected to %s\r\n", ap_info.ssid);
            usbio_print(100, pbuf);
            snprintf(pbuf, sizeof(pbuf), "IP Address:  " IPSTR "\r\n", IP2STR(&ip_info.ip));
            usbio_print(100, pbuf);
            snprintf(pbuf, sizeof(pbuf), "Netmask:     " IPSTR "\r\n", IP2STR(&ip_info.netmask));
            usbio_print(100, pbuf);
            snprintf(pbuf, sizeof(pbuf), "Gateway:     " IPSTR "\r\n", IP2STR(&ip_info.gw));
            usbio_print(100, pbuf);

            // Print DNS (usually available on esp_netif)
            esp_netif_dns_info_t dns_info;
            if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
                snprintf(pbuf, sizeof(pbuf), "DNS Main:    " IPSTR "\r\n", IP2STR(&dns_info.ip.u_addr.ip4));
                usbio_print(100, pbuf);
            }
            if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
                snprintf(pbuf, sizeof(pbuf), "DNS Backup:  " IPSTR "\r\n", IP2STR(&dns_info.ip.u_addr.ip4));
                usbio_print(100, pbuf);
            }
            if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_FALLBACK, &dns_info) == ESP_OK) {
                snprintf(pbuf, sizeof(pbuf), "DNS Fallback:" IPSTR "\r\n", IP2STR(&dns_info.ip.u_addr.ip4));
                usbio_print(100, pbuf);
            }
        } else {
            usbio_print(100, "Status:      Connected (Netif Info Not Found)\r\n");
        }
        usbio_print(100, "========================================\r\n");

        while (1) {
            usbio_print(100, "\r\n[1] Test Network\r\n");
            usbio_print(100, "[2] Disconnect\r\n");
            usbio_print(100, "[3] Return\r\n");
            usbio_print(100, "Please select an option: ");

            char opt_buf[16] = {0};
            wait_for_input_ns(opt_buf, sizeof(opt_buf));
            usbio_print(100, "\r\n");

            if (strcmp(opt_buf, "1") == 0) {
                usbio_print(100, "Enter URL (default: http://www.baidu.com): ");
                char url_buf[256];
                memset(url_buf, 0, sizeof(url_buf));
                wait_for_input_ns(url_buf, sizeof(url_buf));
                trim_whitespace(url_buf);
                usbio_print(100, "\r\n");
                
                if (strlen(url_buf) == 0) {
                    strcpy(url_buf, "http://www.baidu.com");
                }

                esp_http_client_config_t config = {
                    .url = url_buf,
                    .timeout_ms = 10000,
                    .crt_bundle_attach = esp_crt_bundle_attach,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client != NULL) {
                    usbio_print(100, "Requesting: ");
                    usbio_print(100, url_buf);
                    usbio_print(100, "\r\n");

                    esp_err_t err = esp_http_client_perform(client);
                    int status_code = esp_http_client_get_status_code(client);
                    
                    if (err == ESP_OK || status_code > 0) {
                        snprintf(pbuf, sizeof(pbuf), "HTTP Status = %d, content_length = %lld\r\n",
                                 status_code,
                                 esp_http_client_get_content_length(client));
                        usbio_print(100, pbuf);
                    } else {
                        snprintf(pbuf, sizeof(pbuf), "HTTP connection failed. Error: %s\r\n", esp_err_to_name(err));
                        usbio_print(100, pbuf);
                    }
                    esp_http_client_cleanup(client);
                } else {
                    usbio_print(100, "Failed to initialize HTTP client.\r\n");
                }
            } else if (strcmp(opt_buf, "2") == 0) {
                esp_wifi_disconnect();
                usbio_print(100, "WiFi disconnected.\r\n");
                break;
            } else if (strcmp(opt_buf, "3") == 0) {
                break;
            } else {
                usbio_print(100, "Invalid option.\r\n");
            }
        }
    } else {
        usbio_print(100, "Status:      No WiFi connection\r\n");
        usbio_print(100, "========================================\r\n");
        wait_for_enter();
    }
}
