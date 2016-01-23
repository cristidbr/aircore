#include "esp_setup_access.h"
#include "esp_setup_configuration.h"
#include "esp_flash_save.c"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"

#include "user_interface.h"


/**
 * Setup access connection
 * @user_init
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_access( void )
{
    char *ssid, *password;
    // reserve memory for ssid and password
    ssid = ( char* ) os_zalloc( WIFI_SSID_LENGTH * sizeof( char ) );
    password = ( char* ) os_zalloc( WIFI_PASSWORD_LENGTH * sizeof( char ) );
    // retrieve stored wifi configuration
    esp_wifi_get_access_config( ssid, password );
    // setup wifi connection
    esp_wifi_setup_operation_mode();
    esp_wifi_setup_access_connection( ssid, password, WIFI_ACCESS_CACHE_SIZE );
    // free memory
    os_free( ssid );
    os_free( password );
} 

/**
 * Setup operation mode to STATION + AP
 * @user_init
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_operation_mode( void )
{
    // check if already in correct mode
    if( wifi_get_opmode() != WIFI_STATIONAP_MODE )
    {
        wifi_set_opmode( WIFI_STATIONAP_MODE );
        wifi_set_opmode_current( WIFI_STATIONAP_MODE );
    }
} 


/**
 * Set WiFi access configuration for STATION mode
 * @user_init
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_setup_access_connection( char* ssid, char* password, uint8_t ap_cache_size )
{
    struct station_config config;
    // set caching for access point settings
    wifi_station_ap_number_set( ( ap_cache_size > 5 ) ? 5 : ap_cache_size );
    // disable auto connect
    if( wifi_station_get_auto_connect() )
    {
        wifi_station_set_auto_connect( 0 );
        system_restart();
    }
    // enable reconnection to wifi access point
    wifi_station_set_reconnect_policy( true );
    // enable DHCP client
    if( wifi_station_dhcpc_status() == DHCP_STOPPED )
    {
        wifi_station_dhcpc_start();
    }
    // configure access options
    config.bssid_set = 0;
    os_memset( &( config.ssid ), 0x00, WIFI_SSID_LENGTH );
    os_memset( &( config.password ), 0x00, WIFI_PASSWORD_LENGTH );
    os_memcpy( &( config.ssid ), ssid, os_strlen( ssid ) );
    os_memcpy( &( config.password ), password, os_strlen( ssid ) );
    wifi_station_set_config( &config );
    wifi_station_set_config_current( &config );
}

/**
 * Returns the saved WiFi configuration
 */
LOCAL void ICACHE_FLASH_ATTR esp_wifi_get_access_config( char* ssid, char* password )
{
    char ssid_src[ WIFI_SSID_LENGTH ] = WIFI_ACCESS_SSID;
    char password_src[ WIFI_PASSWORD_LENGTH ] = WIFI_ACCESS_PASSWORD;
    // check if parameters don't exist
    if( esp_flash_read_parameter( ESP_WIFI_SSID_ACCESS_ID, ssid_src ) == 0x00 )
    {
        // save parameter defaults in flash
        esp_flash_save_parameter( ESP_WIFI_SSID_ACCESS_ID, ssid_src, os_strlen( ssid_src ) );
        esp_flash_save_parameter( ESP_WIFI_PASSWORD_ACCESS_ID, password_src, os_strlen( password_src ) );
    } else {
        // read the other parameter
        esp_flash_read_parameter( ESP_WIFI_PASSWORD_ACCESS_ID, password_src );
    }
    // copy defined WiFi configuration into the given memory space
    os_memcpy( ssid, ssid_src, os_strlen( ssid_src ) );
    os_memcpy( password, password_src, os_strlen( password_src ) );
}









