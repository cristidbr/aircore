/**
 * \brief		URL Interface
 * \file		esp_url.c
 * \author		Cristian Dobre
 * \version 		1.0.2
 * \date 		February 2016
 * \copyright 		Revised BSD License.
 */
#ifndef __ESP_URL_PARSE_C__
#define __ESP_URL_PARSE_C__

#include "osapi.h"
#include "user_interface.h"
#include "mem.h"

#include "esp_url.h"


/**
 * Returns the encoded length of the parameter string
 */
uint16_t ICACHE_FLASH_ATTR url_parameter_get_encoded_size( uint8_t* parameter_value )
{
    uint16_t length = strlen( ( char* ) parameter_value );
    char c;

    for( ; *( parameter_value ) != 0x00 ; parameter_value++ ) {
        c = *( parameter_value );
        if( ( ! isalnum( c ) ) && ( c != '_' ) && ( c != '-' ) && ( c != '.' ) && ( c != '~' ) && ( c != ' ' ) )
            length++;
    }
    return length;
}

/**
 * Encodes string values according to URI standards
 */
void ICACHE_FLASH_ATTR url_parameter_encode( uint8_t* encoded_parameter, uint8_t* parameter_value )
{
    char hex[ 17 ] = "0123456789ABCDEF", c;
    uint8_t* current_ch = parameter_value;

    for( ; *( current_ch ) != 0x00; current_ch++ ) {
        c = ( char ) *( current_ch );

        if( isalnum( c ) || c == '_' || c == '-' || c == '.' || c == '~' ) {
            *( encoded_parameter++ ) = ( uint8_t ) c;
        } else if( c == ' ' ) {
            *( encoded_parameter++ ) = '+';
        } else {
            *( encoded_parameter++ ) = '%';
            *( encoded_parameter++ ) = hex[ c >> 4 ];
            *( encoded_parameter++ ) = hex[ c & 0x0F ];
        }
    }
    *( encoded_parameter ) = '\0';
}

/**
 * Decodes URI string
 */
void ICACHE_FLASH_ATTR url_parameter_decode( uint8_t* parameter, uint8_t* encoded_parameter )
{
    uint8_t c, d;

    for( ; *( encoded_parameter ) != 0x00 ; encoded_parameter ++ ) {
        if( *( encoded_parameter ) != '%' ) {
            if( *( encoded_parameter ) == '+' ) {
                *( parameter++ ) = ' ';
            } else {
                *( parameter++ ) = *( encoded_parameter );
            }
        } else {
            c = *( ++ encoded_parameter );
            if( c >= '0' && c <= '9' ) d = ( c - '0' ) << 4;
            else d = ( 10 + toupper( c ) - 'A' ) << 4;
            c = *( ++ encoded_parameter );
            if( c >= '0' && c <= '9' ) d |= ( c - '0' ) & 0x0F;
            else d |= ( 10 + toupper( c ) - 'A' ) & 0x0F;
            *( parameter++ ) = d;
        }
    }
    *( parameter ) = '\0';
}

/**
 * Deinitialize URL parameter and frees the memory
 */
void ICACHE_FLASH_ATTR url_query_parameter_destroy( url_query_parameter_type* parameter )
{
    if( parameter->name != NULL )
        os_free( parameter->name );
    if( parameter->value != NULL )
        os_free( parameter->value );
    os_free( parameter );
}


/**
 * Initialize a query parameter
 */
void ICACHE_FLASH_ATTR url_query_parameter_initialize( url_query_parameter_type* parameter, uint8_t* parameter_name, uint8_t* parameter_value, uint8_t encoded_value )
{
    char* value;

    if( parameter->name != NULL ) {
        os_free( parameter->name );
        parameter->name = NULL;
    }
    if( parameter->value != NULL ) {
        os_free( parameter->value );
        parameter->value = NULL;
    }

    parameter->name = ( uint8_t* ) os_malloc( strlen( ( char* ) parameter_name ) + 1 );
    strcpy( ( char* ) parameter->name, ( char* ) parameter_name );

    value = ( char* ) os_malloc( strlen( ( char* ) parameter_value ) + 1 );

    if( encoded_value ) {
        url_parameter_decode( ( uint8_t* ) value, parameter_value );
        parameter->value = ( uint8_t* ) os_malloc( strlen( value ) + 1 );
        strcpy( ( char* ) parameter->value, value );
        os_free( value );
    } else {
        strcpy( ( char* ) value, ( char* ) parameter_value );
        parameter->value = ( uint8_t* ) value;
    }
}

/**
 * Get query length
 */
uint32_t ICACHE_FLASH_ATTR url_query_compute_length( url_query_parameter_type* list )
{
    uint32_t length = 0, i = 0;

    while( list != NULL ) {
        i++;
        length += strlen( ( char* ) list->name ) + url_parameter_get_encoded_size( list->value ) + 1;
        list = list->chain;
    }
    if( i != 0 ) i--;
    return length + i;
}

/**
 * Build a query parameter
 */
url_query_parameter_type* ICACHE_FLASH_ATTR url_query_parameter_create( uint8_t* parameter_name, uint8_t* parameter_value, uint8_t encoded_value )
{
    url_query_parameter_type* p = ( url_query_parameter_type* ) os_malloc( sizeof( url_query_parameter_type ) );

    p->chain = NULL;
    p->name = NULL;
    p->value = NULL;
    url_query_parameter_initialize( p, parameter_name, parameter_value, encoded_value );

    return p;
}


/**
 * Finds URL query parameter by name, returns the linking parameter in the chain
 */
url_query_parameter_type* ICACHE_FLASH_ATTR url_find_query_parameter( url_query_parameter_type* list, int8_t* parameter_index, uint8_t* parameter_name )
{
    url_query_parameter_type* link = list;
    url_query_parameter_type* s;

    *( parameter_index ) = -1;
    if( link == NULL ) return NULL;
    if( stricmp( ( char* ) parameter_name, ( char* ) link->name ) == 0 ) {
        ( *parameter_index ) ++;
        return list;
    }

    while( link->chain != NULL ) {
        ( *parameter_index ) ++;
        s = ( link->chain );
        if( stricmp( ( char* ) parameter_name, ( char* ) ( s->name ) ) == 0 ) {
            return link;
        }
        link = link->chain;
    }

    ( *parameter_index ) = -1;
    return NULL;
}

/**
 * Insert or update query parameter
 */
void ICACHE_FLASH_ATTR url_add_query_parameter( url_object_type* url, uint8_t* parameter_name, uint8_t* parameter_value, uint8_t encoded_value )
{
    url_query_parameter_type* list = url->query;
    url_query_parameter_type* search;
    url_query_parameter_type* insert = NULL;
    int8_t parameter_index;

    search = url_find_query_parameter( list, &parameter_index, parameter_name );
    if( parameter_index == -1 ) {
        insert = url_query_parameter_create( parameter_name, parameter_value, encoded_value );
        search = list;
        if( search == NULL ) {
            url->query = insert;
        } else {
            while( search->chain != NULL )
                search = search->chain;
            search->chain = insert;
        }
    } else {
        insert = search->chain;
        url_query_parameter_initialize( insert, parameter_name, parameter_value, encoded_value );
    }
}

/**
 * Get the query parameter
 */
url_query_parameter_type* ICACHE_FLASH_ATTR url_get_query_parameter( url_object_type* url, uint8_t* parameter_name )
{
    url_query_parameter_type* search;
    int8_t parameter_index;
    search = url_find_query_parameter( url->query, &parameter_index, parameter_name );
    if( parameter_index > 0 )
        search = search->chain;
    return search;
}

/**
 * Remove the query parameter from the parameters list
 */
void ICACHE_FLASH_ATTR url_remove_query_parameter( url_object_type* url, uint8_t* parameter_name )
{
    url_query_parameter_type* list = url->query;
    url_query_parameter_type* search;
    int8_t parameter_index;

    search = url_find_query_parameter( list, &parameter_index, parameter_name );
    if( parameter_index == 0 ) {
        url->query = list->chain;
        url_query_parameter_destroy( list );
    } else if( parameter_index > 0 ) {
        list = search->chain;
        search->chain = list->chain;
        url_query_parameter_destroy( list );
    }
}

/**
 * Clean URL query
 */
void ICACHE_FLASH_ATTR url_remove_query( url_object_type* url )
{
    url_query_parameter_type* list = url->query, *next;

    while( list != NULL ) {
        next = list->chain;
        url_query_parameter_destroy( list );
        list = next;
    }
    url->query = NULL;
}


/**
 * Write the current query into a string
 */
uint8_t* ICACHE_FLASH_ATTR url_get_query( uint8_t* destination, url_object_type* url )
{
    url_query_parameter_type* list = url->query;

    while( list != NULL ) {
        strcpy( ( char* ) destination, ( char* ) list->name );
        destination = destination + strlen( ( char* ) destination );
        *( destination++ ) = '=';
        url_parameter_encode( destination, list->value );
        destination = destination + strlen( ( char* ) destination );

        list = list->chain;
        if( list != NULL )
            *( destination++ ) = '&';
    }
    return destination + strlen( ( char* ) destination );
}


/**
 * Write IP as string in the specified location
 */
void ICACHE_FLASH_ATTR url_ip_to_hostname( uint8_t* hostname, uint32_t ip )
{
    uint8_t* p = ( uint8_t* ) ( &ip );
    os_sprintf( ( char* ) hostname, "%d.%d.%d.%d", p[ 3 ], p[ 2 ], p[ 1 ], p[ 0 ] );
}

/**
 * Check if the given string is an IP
 */
uint32_t ICACHE_FLASH_ATTR url_hostname_is_ip( uint8_t* hostname )
{
    uint8_t ac = 0, ip[ 4 ];
    uint8_t i, n;
    uint16_t bval = 0;
    uint32_t address = 0x00000000;
    char c;

    if( hostname == NULL ) return address;
    n = strlen( ( char* ) hostname );

    for( i = 0; i < n ; i++ ) {
        c = ( char ) ( *( hostname + i ) );

        if( isdigit( c ) ) {
            bval = ( bval * 10 ) + ( c - '0' );
        } else if( c == '.' ) {
            if( ++ ac == 4 ) break;
            if( bval <= 0xFF )
                ip[ ac - 1 ] = ( uint8_t ) bval;
            else break;
            bval = 0x00;
        } else {
            break;
        }
    }

    if( i != n ) return address;
    if( bval <= 0xFF && bval > 0x00 )
        ip[ ac ] = ( uint8_t ) bval;

    address |= ( (( uint32_t )( ip[ 0 ] )) << 24 ) | ( (( uint32_t )( ip[ 1 ] )) << 16 ) | ( (( uint32_t )( ip[ 2 ] )) << 8 ) | ip[ 3 ];
    return address;
}

/**
 * Parses URL query into usable objects
 */
void ICACHE_FLASH_ATTR url_parse_query( url_object_type* url, uint8_t* query, int16_t length )
{
    url_query_parameter_type* list = url->query;
    uint8_t value[ 512 ], *parse = query, *eq = NULL, *name = query, *fragment, *end = query + ( length - 1 );
    uint16_t copy_length;
    char c;
    
    if( length > 0 ) {
        
        while( parse != end ) {
            if( ( *parse ) == '#' ) {
                fragment = parse;
                ( *fragment ) = '\0';
            }
            parse++;
        }
        parse = query;    

        for( ; parse <= end ; parse++ ) {
            c = ( char ) ( *parse );
            if( parse == end ) {
                copy_length = ( int16_t ) ( end - eq );
                strncpy( value, eq + 1, copy_length );
                value[ copy_length ] = '\0';
                if( strlen( ( char* )( value ) ) != 0 && strlen( ( char* )( name ) ) != 0 )
                    url_add_query_parameter( url, ( uint8_t* ) name, ( uint8_t* ) eq + 1, 1 );
                ( *eq ) = '=';
            } else if( c == '&' ) {
                ( *parse ) = '\0';
                if( strlen( ( char* )( eq + 1 ) ) != 0 && strlen( ( char* )( name ) ) != 0 )
                    url_add_query_parameter( url, ( uint8_t* ) name, ( uint8_t* ) eq + 1, 1 );
                ( *parse ) = '&';
                ( *eq ) = '=';
                name = parse + 1;
                while( ( *name ) == '=' || ( *name ) == '&' ) {
                    name++;
                    parse++;
                }
            } else if( c == '=' ) {
                eq = parse;
                ( *parse ) = '\0';
            }
        }

        if( fragment != NULL )
            ( *fragment ) = '#';
    }
}

/**
 * Initializes an URL object
 */
void ICACHE_FLASH_ATTR url_initialize( url_object_type* url, url_protocol_type protocol, uint8_t* hostname, uint16_t port, uint8_t* path, uint8_t* query )
{
    url->protocol = protocol;
    url->port = port;
    url->query = NULL;

    url->host_ip = url_hostname_is_ip( hostname );
    if( url->host_ip == 0x00000000 ) {
        if( hostname == NULL ) url->hostname = NULL;
        else {
            url->hostname = ( uint8_t* ) os_malloc( strlen( ( char* ) hostname ) + 1 );
            strcpy( ( char* ) url->hostname, ( char* ) hostname );
        }
    }

    if( path == NULL ) url->path = NULL;
    else {
        url->path = ( uint8_t* ) os_malloc( strlen( ( char* ) path ) + 1 );
        strcpy( ( char* ) url->path, ( char* ) path );
    }
    if( strlen( query ) > 0 ) 
        url_parse_query( url, query, strlen( query ) );
    else 
        url->query = NULL;
}

/**
 * Destroy the URL object and frees up the memory
 */
void ICACHE_FLASH_ATTR url_deinitialize( url_object_type* url )
{
    url_remove_query( url );

    os_free( url->hostname );
    os_free( url->path );
    url->hostname = NULL;
    url->path = NULL;
}


/**
 * Match URL protocol
 */
url_protocol_type ICACHE_FLASH_ATTR url_get_protocol_type( uint8_t* protocol )
{
    char https[ 6 ] = "http\0\0";
    char wss[ 4 ] = "ws\0\0";
    url_protocol_type p = URL_PROTOCOL_NONE;

    if( stricmp( ( char* ) protocol, https ) == 0 ) {
        p = URL_PROTOCOL_HTTP;
    } else {
        https[ 4 ] = 's';
        if( stricmp( ( char* ) protocol, https ) == 0 ) {
            p = URL_PROTOCOL_HTTPS;
        } else {
            if( stricmp( ( char* ) protocol, wss ) == 0 ) {
                p = URL_PROTOCOL_WS;
            } else {
                wss[ 2 ] = 's';
                if( stricmp( ( char* ) protocol, wss ) == 0 )
                    p = URL_PROTOCOL_WSS;
            }
        }
    }

    return p;
}


/**
 * Writes the protocol type at the given location
 */
void ICACHE_FLASH_ATTR url_write_protocol_type( uint8_t* destination, url_protocol_type protocol )
{
    char https[ 6 ] = "http\0\0";
    char wss[ 4 ] = "ws\0\0";

    if( protocol != URL_PROTOCOL_NONE ) {
        if( protocol == URL_PROTOCOL_HTTP ) {
            strcpy( ( char* ) destination, https );
        } else if( protocol == URL_PROTOCOL_HTTPS ) {
            https[ 4 ] = 's';
            strcpy( ( char* ) destination, https );
        } else if( protocol == URL_PROTOCOL_WS ) {
            strcpy( ( char* ) destination, wss );
        } else {
            wss[ 2 ] = 's';
            strcpy( ( char* ) destination, wss );
        }
    }
}

/**
 * Advance until whitespace ends
 */
uint8_t* ICACHE_FLASH_ATTR skip_whitespace( uint8_t* data )
{
    while( isspace( *( data ) ) )
        data++;
    return data;
}

/**
 * Parse URL path and query into object
 */
void ICACHE_FLASH_ATTR url_parse_path( url_object_type* url, uint8_t* url_path )
{
    uint8_t* fragment, *delimiter, *query;

    url_path = skip_whitespace( url_path );

    if( ( fragment = ( uint8_t* ) strchr( ( char* ) url_path, '#' ) ) != NULL ) ( *fragment ) = '\0';
    if( ( delimiter = ( uint8_t* ) strchr( ( char* ) url_path, '?' ) ) != NULL ) {
        ( *delimiter ) = '\0';
        query = delimiter + 1;
        url_parse_query( url, query, strlen( query ) );
    }

    if( url->path != NULL ) os_free( url_path );
    url->path = ( uint8_t* ) os_malloc( strlen( ( char* ) url_path ) + 1 );
    strcpy( ( char* ) url->path, ( char* ) url_path );

    if( fragment != NULL ) ( *fragment ) = '#';
    if( delimiter != NULL ) ( *delimiter ) = '?';
}


/**
 * Instantiates URL object from string
 */
void ICACHE_FLASH_ATTR url_parse( url_object_type* url, uint8_t* url_string )
{
    char hostname[ 256 ] = "";
    uint8_t* delimiter, *parse, *query, *port_syntax, *path;
    uint16_t port = 0;

    url->protocol = URL_PROTOCOL_NONE;

    if( ( *( url_string ) == '/' ) && ( *( url_string + 1 ) == '/' ) )
        url_string += 2;

    if( ( delimiter = ( uint8_t* ) strchr( ( char* ) url_string, ':' ) ) != NULL ) {
        if( ( *( delimiter + 1 ) == '/' ) && ( *( delimiter + 2 ) == '/' ) ) {
            ( *delimiter ) = '\0';
            url->protocol = url_get_protocol_type( url_string );
            ( *delimiter ) = ':';
            url_string = delimiter + 3;
        }
    }

    if( ( delimiter = ( uint8_t* ) strchr( ( char* ) url_string, '/' ) ) != NULL )
        ( *delimiter ) = '\0';

    if( ( parse = ( uint8_t* ) strchr( ( char* ) url_string, '@' ) ) != NULL )
        url_string = parse + 1;

    if( ( port_syntax = ( uint8_t* ) strchr( ( char* ) url_string, ':' ) ) != NULL ) {
        ( *port_syntax ) = '\0';
        for( parse = port_syntax + 1; ( *parse ) != '\0' ; parse++ )
            if( isdigit( *parse ) )
                port = ( port * 10 ) + (( *parse ) - '0' );
    }
    strcpy( hostname, ( char* ) url_string );
    if( port_syntax != NULL )
        ( *port_syntax ) = ':';

    if( delimiter != NULL ) {
        ( *delimiter ) = '/';
        url_string = path = delimiter;
    } else {
        url_string = path = query = url_string + strlen( ( char* ) url_string );
    }

    if( ( port_syntax = ( uint8_t* ) strchr( ( char* ) url_string, '#' ) ) != NULL )
        ( *port_syntax ) = '\0';

    if( ( delimiter = ( uint8_t* ) strchr( ( char* ) url_string, '?' ) ) != NULL ) {
        ( *delimiter ) = '\0';
        query = delimiter + 1;
    } else {
        query = url_string + strlen( ( char* ) url_string );
    }

    url_initialize( url, url->protocol, ( uint8_t* ) hostname, port, path, query );

    if( port_syntax != NULL ) ( *port_syntax ) = '#';
    if( delimiter != NULL ) ( *delimiter ) = '?';
}

/**
 * Returns a full URI address, built from the given URL object
 */
void ICACHE_FLASH_ATTR url_get_string( uint8_t* query, url_object_type* url, uint8_t show_port )
{
    char delimiter[ 4 ] = "://";
    uint8_t has_host = 0;

    if( url->protocol != URL_PROTOCOL_NONE ) {
        url_write_protocol_type( query, url->protocol );
        query = query + strlen( ( char* ) query );
        strcpy( ( char* ) query, delimiter );
        query = query + strlen( ( char* ) query );
    }

    if( url->hostname != NULL && ( strlen( ( char* ) url->hostname ) > 0 ) ) {
        strcpy( ( char* ) query, ( char* ) url->hostname );
        has_host = 1;
    } else if( url->host_ip != 0x00000000 ) {
        url_ip_to_hostname( query, url->host_ip );
        has_host = 1;
    }

    if( has_host ) {
        query = query + strlen( ( char* ) query );
        if( show_port && url->port != 0x0000 ) {
            os_sprintf( ( char* ) query, ":%d", url->port );
            query = query + strlen( ( char* ) query );
        }
    }

    if( url->path != NULL && ( strlen( ( char* ) url->path ) > 0 ) ) {
        strcpy( ( char* ) query, ( char* ) url->path );
        query = query + strlen( ( char* ) query );
    }

    if( url->query != NULL ) {
        *( query++ ) = '?';
        url_get_query( query, url );
    }

}


#endif
