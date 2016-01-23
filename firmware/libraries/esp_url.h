/**
 * \brief		URL Parser and Constructor Functions
 * \file		esp_url.h
 * \author		Cristian Dobre
 * \version 	1.0.0
 * \date 		November 2015
 * \copyright 	Revised BSD License.
 */
#ifndef __ESP_URL_PARSE_H__
#define __ESP_URL_PARSE_H__

#include "os_type.h"

/**
 * URL Schemes
 */
typedef enum {
    URL_PROTOCOL_NONE = 0,
    URL_PROTOCOL_HTTP,
    URL_PROTOCOL_HTTPS,
    URL_PROTOCOL_WS,
    URL_PROTOCOL_WSS
} url_protocol_type;


/**
 * Parameter list structure
 */
typedef struct url_query_parameter url_query_parameter_type;

struct url_query_parameter {
    uint8_t* name;
    uint8_t* value;
    url_query_parameter_type* chain;
};

/**
 * URL Object
 */
typedef struct url_object {
    url_protocol_type protocol;
    uint32_t host_ip;
    uint8_t* hostname;
    uint16_t port;
    uint8_t* path;
    url_query_parameter_type* query;
} url_object_type;


/**
 * URL Constructor functions
 */
LOCAL void ICACHE_FLASH_ATTR url_initialize( url_object_type*, url_protocol_type protocol, uint8_t* hostname, uint16_t port, uint8_t* path, uint8_t* query );
LOCAL void ICACHE_FLASH_ATTR url_parse( url_object_type* url, uint8_t* url_string );
LOCAL void ICACHE_FLASH_ATTR url_parse_query( url_object_type*, uint8_t* query );
LOCAL void ICACHE_FLASH_ATTR url_get_string( uint8_t* query, url_object_type* url, uint8_t show_port );

/**
 * URL Query parameter functions
 */
LOCAL void ICACHE_FLASH_ATTR url_parameter_encode( uint8_t* encoded_parameter, uint8_t* parameter_value );
LOCAL void ICACHE_FLASH_ATTR url_parameter_decode( uint8_t* parameter, uint8_t* encoded_parameter );

LOCAL void ICACHE_FLASH_ATTR url_add_query_parameter( url_object_type*, uint8_t* parameter_name, uint8_t* parameter_value, uint8_t encoded_value );
LOCAL url_query_parameter_type* ICACHE_FLASH_ATTR url_get_query_parameter( url_object_type* url, uint8_t* parameter_name );
LOCAL void ICACHE_FLASH_ATTR url_remove_query_parameter( url_object_type*, uint8_t* parameter_name );

/**
 * URL Cleanup functions
 */
LOCAL void ICACHE_FLASH_ATTR url_remove_query( url_object_type* );
LOCAL void ICACHE_FLASH_ATTR url_deinitialize( url_object_type* );

#endif
