/**
 * \brief		URL Parser and Constructor Functions
 * \file		esp_url.h
 * \author		Cristian Dobre
 * \version 		1.0.2
 * \date 		February 2016
 * \copyright 		Revised BSD License.
 */
#ifndef __ESP_URL_PARSE_H__
#define __ESP_URL_PARSE_H__

#include "osapi.h"
#include "user_interface.h"

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
void ICACHE_FLASH_ATTR url_initialize( url_object_type*, url_protocol_type protocol, uint8_t* hostname, uint16_t port, uint8_t* path, uint8_t* query );
void ICACHE_FLASH_ATTR url_parse( url_object_type* url, uint8_t* url_string );
void ICACHE_FLASH_ATTR url_parse_path( url_object_type* url, uint8_t* url_path );
void ICACHE_FLASH_ATTR url_parse_query( url_object_type*, uint8_t* query, int16_t length );
void ICACHE_FLASH_ATTR url_get_string( uint8_t* query, url_object_type* url, uint8_t show_port );

/**
 * URL Query parameter functions
 */
void ICACHE_FLASH_ATTR url_parameter_encode( uint8_t* encoded_parameter, uint8_t* parameter_value );
void ICACHE_FLASH_ATTR url_parameter_decode( uint8_t* parameter, uint8_t* encoded_parameter );

void ICACHE_FLASH_ATTR url_add_query_parameter( url_object_type*, uint8_t* parameter_name, uint8_t* parameter_value, uint8_t encoded_value );
url_query_parameter_type* ICACHE_FLASH_ATTR url_get_query_parameter( url_object_type* url, uint8_t* parameter_name );
void ICACHE_FLASH_ATTR url_remove_query_parameter( url_object_type*, uint8_t* parameter_name );
uint8_t* ICACHE_FLASH_ATTR url_get_query( uint8_t* destination, url_object_type* url );
uint32_t ICACHE_FLASH_ATTR url_query_compute_length( url_query_parameter_type* list );

/**
 * IP Functions
 */
void ICACHE_FLASH_ATTR url_ip_to_hostname( uint8_t* hostname, uint32_t ip );

/**
 * URL Cleanup functions
 */
void ICACHE_FLASH_ATTR url_remove_query( url_object_type* );
void ICACHE_FLASH_ATTR url_deinitialize( url_object_type* );

uint8_t* ICACHE_FLASH_ATTR skip_whitespace( uint8_t* data );

#endif
