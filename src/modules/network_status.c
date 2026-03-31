#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include <utils.h>
#include <usb_serial_io.h>

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
        } else {
            usbio_print(100, "Status:      Connected (Netif Info Not Found)\r\n");
        }
    } else {
        usbio_print(100, "Status:      No WiFi connection\r\n");
    }

    usbio_print(100, "========================================\r\n");
    wait_for_enter();
}
