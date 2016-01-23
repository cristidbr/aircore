#ifndef __ESP_WS_WIFI_ACCESS_H__
#define __ESP_WS_WIFI_ACCESS_H__

#include "os_type.h"
#include "user_interface.h"

LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_access( void );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_get_access_config( char* ssid, char* password );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_operation_mode( void );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_access_connection( char* ssid, char* password, uint8_t ap_cache_size );

#endif
