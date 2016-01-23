#include "esp_setup_apmode.h"
#include "esp_setup_configuration.h"
#include "esp_flash_save.c"

#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"

/**
 * Configure AP mode
 * @user_init
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode( void )
{
    struct softap_config admin_config;

    // set to 802.11g so AP mode can work
	wifi_set_phy_mode( PHY_MODE_11G );

    // setup IP settings for access point
    esp_wifi_setup_apmode_ip();
 esp_wifi_setup_dhcp_server();
    // setup apmode login
    esp_wifi_setup_apmode_connection( &admin_config );
   
    // set wifi max connections
	admin_config.channel = 4;
	admin_config.max_connection = 4;
    // save admin configuration
	wifi_softap_set_config( &admin_config );
}


/**
 * Setup IP settings
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode_ip( void )
{
    struct ip_info admin_ip_config;  
    uint8 ap_mac[ 6 ] = { WIFI_AP_MAC_ADDRESS };
    uint8 ap_ip[ 4 ] = { WIFI_AP_IP_SELF };
    uint8 ap_gw[ 4 ] = { WIFI_AP_IP_GATEWAY };
    uint8 ap_mk[ 4 ] = { WIFI_AP_IP_MASK };
    
    // check for MAC address
    if( esp_flash_read_parameter( ESP_WIFI_APMODE_MAC_ID, ap_mac ) == 0x00 )
    {
        // save default MAC into flash
        esp_flash_save_parameter( ESP_WIFI_APMODE_MAC_ID, ap_mac, 0x06 );    
    }
    // set MAC address of access point
    wifi_set_macaddr( SOFTAP_IF, ap_mac );
    // check for stored IP settings
   // if( esp_flash_read_parameter( ESP_WIFI_APMODE_IP_SELF_ID, ap_ip ) == 0x00 )
   // {
        // save parameters
        //esp_flash_save_parameter( ESP_WIFI_APMODE_IP_SELF_ID, ap_ip, 0x04 );
        //esp_flash_save_parameter( ESP_WIFI_APMODE_IP_GATEWAY_ID, ap_gw, 0x04 );
        //esp_flash_save_parameter( ESP_WIFI_APMODE_IP_MASK_ID, ap_mk, 0x04 );
   // } else {
        // load parameters
        //esp_flash_read_parameter( ESP_WIFI_APMODE_IP_GATEWAY_ID, ap_gw );
        //esp_flash_read_parameter( ESP_WIFI_APMODE_IP_MASK_ID, ap_mk );
   // }
    // set ip settings
	IP4_ADDR( &admin_ip_config.ip, ap_ip[ 0 ], ap_ip[ 1 ], ap_ip[ 2 ], ap_ip[ 3 ] );
	IP4_ADDR( &admin_ip_config.gw, ap_gw[ 0 ], ap_gw[ 1 ], ap_gw[ 2 ], ap_gw[ 3 ] );
	IP4_ADDR( &admin_ip_config.netmask, ap_mk[ 0 ], ap_mk[ 1 ], ap_mk[ 2 ], ap_mk[ 3 ] );
	// set ip configuration for AP mode
    wifi_set_ip_info( SOFTAP_IF, &admin_ip_config );    
}


/**
 * Setup administrative access to the AirCore module
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_apmode_connection( struct softap_config *admin_config )
{
    uint8_t ap_auth = AUTH_WPA_WPA2_PSK;
    uint8_t ap_hidden = 0x00;
    char ap_ssid[ WIFI_SSID_LENGTH ] = WIFI_AP_SSID;
	char ap_password[ WIFI_PASSWORD_LENGTH ] = WIFI_AP_PASSWORD;
    
    // retrieve current configuration
    wifi_softap_get_config( admin_config );
    // check for stored SSID in flash
    /*if( esp_flash_read_parameter( ESP_WIFI_APMODE_SECURITY_ID, &ap_auth ) == 0x00 )
    {
        // save parameters in flash
        esp_flash_save_parameter( ESP_WIFI_APMODE_SECURITY_ID, &ap_auth, 1 );
        esp_flash_save_parameter( ESP_WIFI_APMODE_HIDDEN_ID, &ap_hidden, 1 );
        esp_flash_save_parameter( ESP_WIFI_SSID_APMODE_ID, ap_ssid, os_strlen( ap_ssid ) );
        esp_flash_save_parameter( ESP_WIFI_PASSWORD_APMODE_ID, ap_password, os_strlen( ap_password ) );
    } else {
        // read parameters
        esp_flash_read_parameter( ESP_WIFI_APMODE_HIDDEN_ID, &ap_hidden );
        esp_flash_read_parameter( ESP_WIFI_SSID_APMODE_ID, ap_ssid );
        esp_flash_read_parameter( ESP_WIFI_PASSWORD_APMODE_ID, ap_password );    
    }*/
    // set wifi authentication mode
	admin_config->authmode = ap_auth;
    admin_config->ssid_hidden = ap_hidden;
    admin_config->ssid_len = 0;
    // configure AP settings
	os_memset( admin_config->ssid, 0x00, WIFI_SSID_LENGTH );
	os_memcpy( admin_config->ssid, ap_ssid, admin_config->ssid_len = os_strlen( ap_ssid ) );
	os_memset( admin_config->password, 0x00, WIFI_PASSWORD_LENGTH );
	os_memcpy( admin_config->password, ap_password, os_strlen( ap_password ) );
}


/**
 * Initializes DHCP server
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_dhcp_server( void )
{
    struct dhcps_lease admin_dhcp_config;
    uint8_t dhcp_server = 0x01;
    uint8_t dhcp_start_ip[ 4 ] = { WIFI_DHCP_SERVER_START_IP };
    uint8_t dhcp_end_ip[ 4 ] = { WIFI_DHCP_SERVER_END_IP };
    // check if parameters don't exist
   /* if( esp_flash_read_parameter( ESP_WIFI_APMODE_DHCP_ENABLE_ID, &dhcp_server ) == 0x00 )
    {
        // save defaults in flash
        esp_flash_save_parameter( ESP_WIFI_APMODE_DHCP_ENABLE_ID, &dhcp_server, 0x01 );
        esp_flash_save_parameter( ESP_WIFI_APMODE_DHCP_START_IP_ID, dhcp_start_ip, 0x04 );
        esp_flash_save_parameter( ESP_WIFI_APMODE_DHCP_END_IP_ID, dhcp_end_ip, 0x04 );
    } else {*/
        // stop the DHCP
        if( wifi_softap_dhcps_status() == DHCP_STARTED )
            wifi_softap_dhcps_stop();
        // check if DHCP is enabled
       // if( dhcp_server == 0x01 )         
       // {
            // read configuration details
            //esp_flash_read_parameter( ESP_WIFI_APMODE_DHCP_START_IP_ID, dhcp_start_ip );
           // esp_flash_read_parameter( ESP_WIFI_APMODE_DHCP_END_IP_ID, dhcp_end_ip );
            // configure and turn on DHCP
            IP4_ADDR( &admin_dhcp_config.start_ip, dhcp_start_ip[ 0 ], dhcp_start_ip[ 1 ], dhcp_start_ip[ 2 ], dhcp_start_ip[ 3 ] );
            IP4_ADDR( &admin_dhcp_config.end_ip, dhcp_end_ip[ 0 ], dhcp_end_ip[ 1 ], dhcp_end_ip[ 2 ], dhcp_end_ip[ 3 ] );
            wifi_softap_set_dhcps_lease( &admin_dhcp_config );
            wifi_softap_dhcps_start();
       // }
  //  }
}
