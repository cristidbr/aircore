#ifndef __ESP_WS_WIFI_APMODE__
#define __ESP_WS_WIFI_APMODE__

#include "os_type.h"
#include "user_interface.h"

LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode( void );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode_ip( void );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode_connection( struct softap_config *admin_config );
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_dhcp_server( void );

#endif
