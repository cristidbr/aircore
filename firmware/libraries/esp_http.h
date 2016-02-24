/**
 * \brief		HTTP Communication Interface
 * \file		esp_http.h
 * \author		Cristian Dobre
 * \version 	1.0.0
 * \date 		November 2015
 * \copyright 	Revised BSD License.
 */
#ifndef __ESP_HTTP_H__
#define __ESP_HTTP_H__

#include "osapi.h"
#include "user_interface.h"

#include "esp_url.h"


/**
 * HTTP Request/Response method
 */
typedef enum {
    HTTP_METHOD_NONE = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST
} http_method_type;

/**
 * HTTP Connection types
 */
typedef enum {
    HTTP_CONNECTION_CLOSE = 0,
    HTTP_CONNECTION_KEEPALIVE,
    HTTP_CONNECTION_UPGRADE
} http_connection_type;

/**
 * Header field schemes
 */
typedef enum {
    HTTP_REQUEST_SCHEME = 0x01,
    HTTP_RESPONSE_SCHEME = 0x02,
    WS_REQUEST_SCHEME = 0x04,
    WS_RESPONSE_SCHEME = 0x08
} http_header_scheme_type;

/**
 * HTTP Headers
 */
typedef struct http_header_field http_header_field_type;

struct http_header_field {
    uint8_t* name;
    uint8_t* value;
    http_header_field_type* chain;
};

/**
 * Base object for HTTP request
 */
typedef struct http_request_object {
    http_method_type method;
    url_object_type* location;
    http_header_scheme_type scheme;

    http_connection_type connection;
    uint32_t content_length;

    http_header_field_type* headers;
    uint8_t* content;
} http_request_object_type;


/**
 * Base object for HTTP response
 */
typedef struct http_response_object {
    uint16_t response_code;

    http_connection_type connection;
    http_header_field_type* headers;

    uint32_t content_length;
    uint8_t* content;
} http_response_object_type;

/**
 * Path scheme
 */
typedef struct http_route_header_scheme http_route_header_scheme_type;

struct http_route_header_scheme {
    uint8_t* path;
    http_header_scheme_type scheme;

    http_route_header_scheme_type* chain;
};

void ICACHE_FLASH_ATTR http_header_field_initialize( http_header_field_type* header, uint8_t* name, uint8_t* value );
uint8_t* ICACHE_FLASH_ATTR http_header_field_output( uint8_t* destination, http_header_field_type* header );

http_header_field_type* ICACHE_FLASH_ATTR http_header_field_add( http_header_field_type* list, uint8_t* name, uint8_t* value );
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_get( http_header_field_type* list, uint8_t* name );

uint8_t* ICACHE_FLASH_ATTR http_header_field_parse_name( uint8_t* data, uint8_t* name, uint8_t skip );
uint8_t* ICACHE_FLASH_ATTR http_header_field_parse_value( uint8_t* data, uint8_t* value, uint8_t skip );

void ICACHE_FLASH_ATTR http_request_initialize( http_request_object_type* request );
void ICACHE_FLASH_ATTR http_request_url( http_request_object_type* request, url_object_type* url, http_method_type method );
void ICACHE_FLASH_ATTR http_request_content( http_request_object_type* request, uint8_t* content, uint32_t length );

void ICACHE_FLASH_ATTR http_route_scheme_add( uint8_t* path, http_header_scheme_type scheme );

uint8_t* ICACHE_FLASH_ATTR http_request_generate( uint8_t* text, http_request_object_type* request );
uint8_t* ICACHE_FLASH_ATTR http_request_parse( http_request_object_type* request, uint8_t* data );

#endif
