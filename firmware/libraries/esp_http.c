/**
 * \brief		HTTP Communication Interface
 * \file		esp_http.c
 * \author		Cristian Dobre
 * \version 	1.0.0
 * \date 		August 2015
 * \copyright 	Revised BSD License.
 */
#ifndef __ESP_HTTP_C__
#define __ESP_HTTP_C__

#include "osapi.h"
#include "user_interface.h"
#include "mem.h"

#include "esp_http.h"


/**
 * Server routing scheme
 */
http_route_header_scheme_type* http_request_scheme_route = NULL;

/**
 * Initialize a header fields or reinitialize
 */
void ICACHE_FLASH_ATTR http_header_field_initialize( http_header_field_type* header, uint8_t* name, uint8_t* value )
{
    uint16_t n;

    if( header->name != NULL ) {
        os_free( header->name );
        header->name = NULL;
    }
    if( header->value != NULL ) {
        os_free( header->value );
        header->value = NULL;
    }

    if( name == NULL || ( n = strlen( ( char* ) name )) == 0 ) {
        header->name = NULL;
    } else if( n != 0 ) {
        header->name = ( uint8_t* ) os_malloc( n + 1 );
        strcpy( ( char* ) header->name, ( char* ) name );
    }

    if( value == NULL || ( n = strlen( ( char* ) value )) == 0 ) {
        header->value = NULL;
    } else if( n != 0 ) {
        header->value = ( uint8_t* ) os_malloc( n + 1 );
        strcpy( ( char* ) header->value, ( char* ) value );
    }
}

/**
 * Create and initialize a header field
 */
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_create( uint8_t* name, uint8_t* value )
{
    http_header_field_type* insert = ( http_header_field_type* ) os_malloc( sizeof( http_header_field_type ));

    insert->name = insert->value = NULL;
    insert->chain = NULL;
    http_header_field_initialize( insert, name, value );
    return insert;
}

/**
 * Deinitializes and destroys header field
 */
void ICACHE_FLASH_ATTR http_header_field_destroy( http_header_field_type* header )
{
    if( header->name != NULL )
        os_free( header->name );
    if( header->value != NULL )
        os_free( header->value );
    os_free( header );
}

/**
 * Search for a header field by name
 */
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_find( http_header_field_type* list, int8_t* field_index, uint8_t *field_name )
{
    http_header_field_type* link = list, *s;

    ( *field_index ) = -1;
    if( link == NULL ) return link;
    if( stricmp( ( char* ) field_name, ( char* ) list->name ) == 0 ) {
        ( *field_index ) ++;
        return list;
    }

    while( link->chain != NULL ) {
        ( *field_index ) ++;
        s = ( link->chain );
        if( stricmp( ( char* ) field_name, ( char* ) ( s->name ) ) == 0 )
            return link;
        link = link->chain;
    }

    ( *field_index ) = -1;
    return NULL;
}

/**
 * Add header field into list
 */
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_add( http_header_field_type* list, uint8_t* name, uint8_t* value )
{
    int8_t field_index;
    http_header_field_type* insert;
    http_header_field_type* search;

    search = http_header_field_find( list, &field_index, name );
    if( field_index == -1 ) {
        insert = http_header_field_create( name, value );
        search = list;
        if( list == NULL ) {
            insert->chain = list;
            return insert;
        } else {
            while( search->chain != NULL )
                search = search->chain;
            search->chain = insert;
            return list;
        }
    }
    if( field_index == 0 ) insert = search;
    else insert = search->chain;

    http_header_field_initialize( insert, name, value );
    return list;
}

/**
 * Remove header field
 */
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_remove( http_header_field_type* list, uint8_t* name )
{
    int8_t field_index;
    http_header_field_type* search, *c;

    search = http_header_field_find( list, &field_index, name );
    if( field_index == 0 ) {
        list = search->chain;
        http_header_field_destroy( search );
    } else if( field_index > 0 ) {
        c = search->chain->chain;
        http_header_field_destroy( search->chain );
        search->chain = c;
    }
    return list;
}

/**
 * Get header field value
 */
http_header_field_type* ICACHE_FLASH_ATTR http_header_field_get( http_header_field_type* list, uint8_t* name )
{
    int8_t field_index;
    http_header_field_type* search;

    search = http_header_field_find( list, &field_index, name );
    if( field_index > 0 ) search = search->chain;
    return search;
}

/**
 * Output header field at destination
 */
uint8_t* ICACHE_FLASH_ATTR http_header_field_output( uint8_t* destination, http_header_field_type* header )
{
    strcpy( ( char* ) destination, ( char* ) header->name );
    destination += strlen( ( char* ) destination );
    *( destination++ ) = ':';
    *( destination++ ) = ' ';
    strcpy( ( char* ) destination, ( char* ) header->value );
    return destination + strlen( ( char* ) destination );
}

/**
 * Find next CRLF ending or LF character
 */
uint8_t* ICACHE_FLASH_ATTR get_line_ending( uint8_t* data )
{
    uint8_t* end;
    end = ( uint8_t* ) strchr( ( char* ) data, '\n' );
    if( end != NULL ) {
        if( end != data ) {
            if( *( end - 1 ) == '\r' )
                end--;
        }
    }
    return end;
}

/**
 * Skips CRLF or LF to get to the next line
 */
uint8_t* ICACHE_FLASH_ATTR skip_line_ending( uint8_t* crlf )
{
    if( ( *crlf ) == '\r' ) crlf ++;
    if( ( *crlf ) == '\n' ) crlf ++;
    return crlf;
}

/**
 * Advance until whitespace starts
 */
uint8_t* ICACHE_FLASH_ATTR skip_to_whitespace( uint8_t* data )
{
    while( ! isspace( *( data ) ) )
        data++;
    return data;
}

/**
 * Reverse until no more whitespace is found
 */
uint8_t* ICACHE_FLASH_ATTR skip_end_whitespace( uint8_t* data )
{
    while( isspace( *( data ) ) )
        data--;
    return data;
}

/**
 * Parse uint32
 */
uint8_t* ICACHE_FLASH_ATTR parse_uint32( uint32_t* value, uint8_t* data )
{
    ( *value ) = 0;
    while( isspace( *( data ) ) )
        data++;
    while( isdigit( *( data ) ) ) {
        ( *value ) = (( *value ) * 10 ) + ( ( *data ) - '0' );
        data ++;
    }
    return data;
}

/**
 * Parse name of header field
 */
uint8_t* ICACHE_FLASH_ATTR http_header_field_parse_name( uint8_t* data, uint8_t* name, uint8_t skip )
{
    uint8_t* c, *e, *s, ev;

    s = skip_whitespace( data );
    e = get_line_ending( s );
    if( e != NULL ) {
        ev = ( *e );
        ( *e ) = '\0';
    }
    c = ( uint8_t* ) strchr( ( char* ) data, ':' );
    if( c != NULL ) {
        if( skip == 0x00 ) {
            ( *c ) = '\0';
            strcpy( ( char* ) name, ( char* ) s );
            ( *c ) = ':';
        }
        data = c;
    }
    if( e != NULL ) ( *e ) = ev;
    return data;
}


/**
 * Parse or skip value in header field
 */
uint8_t* ICACHE_FLASH_ATTR http_header_field_parse_value( uint8_t* data, uint8_t* value, uint8_t skip )
{
    uint8_t *e, c, m = 0;

    data = skip_whitespace( data );
    do {
        e = get_line_ending( data );
        if( e != NULL ) {
            c = ( *e );
            ( *e ) = '\0';
            if( skip == 0x00 ) {
                strcpy( ( char* ) value, ( char* ) data );
                value += strlen( ( char* ) value );
            }
            ( *e ) = c;
            data = skip_line_ending( e );
            if(( *data ) == '\t' || ( *data ) == ' ' ) {
                m = 1;
                data++;
            } else m = 0;
        }
        e = NULL;
    } while( m );

    return data;
}

/**
 * Get header value length
 */
uint16_t ICACHE_FLASH_ATTR http_header_field_value_length( uint8_t* data )
{
    uint16_t length = 0;
    uint8_t* d, *e, m, c;

    d = data = skip_whitespace( data );
    do {
        e = get_line_ending( d );
        if( e != NULL ) {
            c = ( *e );
            ( *e ) = '\0';
            length += strlen( ( char* ) d );
            ( *e ) = c;
            d = skip_line_ending( e );
            if(( *d ) == '\t' || ( *d ) == ' ' ) {
                m = 1;
                d++;
            } else m = 0;
        } else m = 0;
        e = NULL;
    } while( m );
    return length;
}

/**
 * Initializes HTTP request objects
 */
void ICACHE_FLASH_ATTR http_request_initialize( http_request_object_type* request )
{
    request->method = HTTP_METHOD_NONE;
    request->location = NULL;
    request->connection = HTTP_CONNECTION_CLOSE;
    request->content = NULL;
    request->content_length = 0;
    request->headers = NULL;
}

/**
 * Use URL for the HTTP request
 */
void ICACHE_FLASH_ATTR http_request_url( http_request_object_type* request, url_object_type* url, http_method_type method )
{
    request->method = method;
    request->location = url;
}


/**
 * Add content to HTTP requests
 */
void ICACHE_FLASH_ATTR http_request_content( http_request_object_type* request, uint8_t* content, uint32_t length )
{
    request->content_length = length;
    request->content = content;
}

/**
 * Output HTTP request to string
 */
uint8_t* ICACHE_FLASH_ATTR http_request_generate( uint8_t* text, http_request_object_type* request )
{
    http_header_field_type* search;
    uint8_t http_ws = 0;
    char *p, method_get[ 5 ] = "GET ", method_post[ 6 ] = "POST ", http_v[ 11 ] = "HTTP/1.0\r\n";
    char cn[ 11 ] = "Connection", cn_close[ 6 ] = "close", cn_keep[ 11 ] = "keep-alive", cn_upgrade[ 8 ] = "Upgrade";
    char host[ 5 ] = "Host", ua[ 11 ] = "User-Agent", ua_v[ 27 ] = "ESPHttp/1.0 (AirCore; 1.0)";
    char accept[ 52 ] = "Accept\0 text/html,application/xhtml+xml,*/*;q=0.8\r\n";
    char content[ 17 ] = "Content-Length: ", ctype[ 50 ] = "Content-Type: application/x-www-form-urlencoded\r\n";
    url_object_type* url = request->location;

    if( request->method != HTTP_METHOD_NONE ) {
        if( request->method == HTTP_METHOD_POST ) strcpy( ( char* ) text, method_post );
        else strcpy( ( char* ) text, method_get );
        text += strlen( ( char* ) text );

        if( url != NULL ) {
            http_ws = url->protocol == URL_PROTOCOL_WS || url->protocol == URL_PROTOCOL_WSS;
            p = ( char* ) url->path;
            if( p != NULL && strlen( p ) != 0 ) {
                strcpy( ( char* ) text, p );
                text += strlen( ( char* ) text );
            } else
                *( text++ ) = '/';

            if( request->method == HTTP_METHOD_GET ) {
                if( request->location->query != NULL ) {
                    *( text++ ) = '?';
                    text = url_get_query( text, url );
                }
            }
        } else *( text++ ) = '/';

        *( text++ ) = ' ';
        if( request->connection != HTTP_CONNECTION_CLOSE || http_ws )
            http_v[ 7 ] = '1';
        strcpy( ( char* ) text, http_v );
        text += strlen( ( char* ) text );

        search = http_header_field_get( request->headers, ( uint8_t* ) host );
        if( search == NULL ) {
            if( url != NULL ) {
                strcpy( ( char* ) text, host );
                text += strlen( ( char* ) text );
                *( text++ ) = ':'; *( text++ ) = ' ';
                if( url->hostname != NULL && ( strlen( ( char* ) url->hostname ) > 0 ) ) strcpy( ( char* ) text, ( char* ) url->hostname );
                else url_ip_to_hostname( text, url->host_ip );
                text += strlen( ( char* ) text );
                *( text++ ) = '\r'; *( text++ ) = '\n';
            }
        }

        search = http_header_field_get( request->headers, ( uint8_t* ) cn );
        if( search == NULL ) {
            strcpy( ( char* ) text, cn );
            text += strlen( ( char* ) text );
            *( text++ ) = ':'; *( text++ ) = ' ';
            if( request->connection == HTTP_CONNECTION_KEEPALIVE ) strcpy( ( char* ) text, cn_keep );
            else if( request->connection == HTTP_CONNECTION_UPGRADE || http_ws ) strcpy( ( char* ) text, cn_upgrade );
            else strcpy( ( char* ) text, cn_close );
            text += strlen( ( char* ) text );
            *( text++ ) = '\r'; *( text++ ) = '\n';
        }

        search = http_header_field_get( request->headers, ( uint8_t* ) ua );
        if( search == NULL ) {
            strcpy( ( char* ) text, ua );
            text += strlen( ( char* ) text );
            *( text++ ) = ':'; *( text++ ) = ' ';
            strcpy( ( char* ) text, ua_v );
            text += strlen( ( char* ) text );
            *( text++ ) = '\r'; *( text++ ) = '\n';
        }

        search = http_header_field_get( request->headers, ( uint8_t* ) accept );
        if( search == NULL ) {
            accept[ 6 ] = ':';
            strcpy( ( char* ) text, accept );
            text += strlen( ( char* ) text );
        }

        search = request->headers;
        while( search != NULL ) {
            text = http_header_field_output( text, search );
            *( text++ ) = '\r'; *( text++ ) = '\n';
            search = search->chain;
        }

        if( ( request->method == HTTP_METHOD_POST && url != NULL ) || ( request->content_length != 0 ) ) {
            if( request->method == HTTP_METHOD_POST ) {
                strcpy( ( char* ) text, ctype );
                text += strlen( ( char* ) text );
                request->content_length = url_query_compute_length( url->query );
            }
            strcpy( ( char* ) text, content );
            text += strlen( ( char* ) text );
            os_sprintf( ( char* ) text, "%d\r\n", request->content_length );
            text += strlen( ( char* ) text );
            *( text++ ) = '\r'; *( text++ ) = '\n';

            if( request->method == HTTP_METHOD_POST ) url_get_query( text, url );
            else strcpy( ( char* ) text, ( char* ) request->content );
            text += strlen( ( char* ) text );
        } else {
            *( text++ ) = '\r'; *( text++ ) = '\n';
        }
    }

    ( *text ) = '\0';
    return text;
}


/**
 * Check if header field value needs to be saved according to the given scheme
 */
uint8_t ICACHE_FLASH_ATTR http_header_field_check_scheme( uint8_t* name, http_header_scheme_type scheme )
{
    char header_upgrade[ 8 ] = "Upgrade", header_sec_ws_key[ 18 ] = "Sec-WebSocket-Key", header_sec_ws_version[ 22 ] = "Sec-WebSocket-Version";
    char header_sec_ws_accept[ 21 ] = "Sec-WebSocket-Accept";
    char content_length[ 15 ] = "Content-Length", hostname[ 5 ] = "Host";

    if( strcmp( hostname, ( char* ) name ) == 0 ) return 0x01;
    if( strcmp( content_length, ( char* ) name ) == 0 ) return 0x01;

    if( scheme & WS_REQUEST_SCHEME ) {
        if( strcmp( header_upgrade, ( char* ) name ) == 0 ) return 0x01;
        if( strcmp( header_sec_ws_key, ( char* ) name ) == 0 ) return 0x01;
        if( strcmp( header_sec_ws_version, ( char* ) name ) == 0 ) return 0x01;
    }
    if( scheme & WS_RESPONSE_SCHEME ) {
        if( strcmp( header_upgrade, ( char* ) name ) == 0 ) return 0x01;
        if( strcmp( header_sec_ws_accept, ( char* ) name ) == 0 ) return 0x01;
    }
    // compare to saved fields
    return 0x00;
}


/**
 * Parses first line of request
 */
uint8_t* ICACHE_FLASH_ATTR http_request_parse_method_line( http_request_object_type* request, uint8_t* data )
{
    uint8_t* w, v, *end, ev;
    char hpost[ 5 ] = "POST", hget[ 4 ] = "GET";

    ev = *( end = get_line_ending( data ));
    ( *end ) = '\0';
    request->method = HTTP_METHOD_NONE;
    data = skip_whitespace( data );
    v = *( w = skip_to_whitespace( data ) );
    ( *w ) = '\0';
    if( strcmp( hget, ( char* ) data ) == 0 ) request->method = HTTP_METHOD_GET;
    if( strcmp( hpost, ( char* ) data ) == 0 ) request->method = HTTP_METHOD_POST;
    ( *w ) = v;

    data = skip_whitespace( w );
    v = *( w = skip_to_whitespace( data ) );
    ( *w ) = '\0';
    url_parse_path( request->location, data );
    ( *w ) = v;
    ( *end ) = ev;
    return skip_line_ending( end );
}


/**
 * Add new routing scheme
 */
void ICACHE_FLASH_ATTR http_route_scheme_add( uint8_t* path, http_header_scheme_type scheme )
{
    http_route_header_scheme_type *p = http_request_scheme_route, *route = ( http_route_header_scheme_type* ) os_malloc( sizeof( http_route_header_scheme_type ) );
	uint8_t* mpath = ( uint8_t* ) os_malloc( sizeof( uint8_t ) * strlen( ( char* ) path ) );
    
    strcpy( ( char* ) mpath, ( char* ) path );
	route->path = mpath;
	route->scheme = scheme;
	route->chain = NULL;
    
    if( p == NULL ) http_request_scheme_route = route;
    else {
        while( p->chain != NULL ) p = p->chain;
        p->chain = route;
    }
}

/**
 * Path checker function, finds request scheme for the given path
 */
http_header_scheme_type ICACHE_FLASH_ATTR http_request_path_get_scheme( uint8_t* path )
{
    http_route_header_scheme_type *p = http_request_scheme_route;
    char index[ 7 ] = "index.", *s, *z = NULL;
    http_header_scheme_type h = 0x00;

    if( ( s = strstr( ( char * ) path, index ) ) != NULL ) {
        *( s ) = '\0'; 
        if( (( s - 1 ) > (( char* ) path ) ) && ( *( s - 1 ) == '/' ) )
            *( z = s - 1 ) = '\0';
    }
    while( p != NULL ) {
        if( strcmp( ( char* ) path, ( char* ) p->path ) == 0 ) {
			h = p->scheme;
            break;
        }
        p = p->chain;
	}
    if( s != NULL ) {
        *( s ) = 'i';
        if( z != NULL ) *( z ) = '/';
    }
	return h;
}


/**
 * HTTP request parsing
 */
uint8_t* ICACHE_FLASH_ATTR http_request_parse( http_request_object_type* request, uint8_t* data )
{
    http_header_scheme_type scheme;
    char empty[ 1 ] = "\0", host[ 5 ] = "Host", content_length[ 15 ] = "Content-Length", header_sec_ws_key[ 18 ] = "Sec-WebSocket-Key";
    uint8_t* mark, *store, *end;
    uint16_t length;

    if( request->location == NULL ) {
        request->location = ( url_object_type* ) os_malloc( sizeof( url_object_type ) );
        url_initialize( request->location, URL_PROTOCOL_NONE, ( uint8_t* ) empty, 0, ( uint8_t* ) empty, ( uint8_t* ) empty );
    }

    data = http_request_parse_method_line( request, data );
    scheme = http_request_path_get_scheme( request->location->path );

    request->content_length = 0;
    request->headers = NULL;
    request->location->hostname = NULL;
    request->location->host_ip = 0x00000000;
    request->scheme = scheme;
    
    while( ( *data != '\0' ) && (( *data ) != '\r' && ( *( data + 1 ) != '\n' )) ) {
        mark = http_header_field_parse_name( data, ( uint8_t* ) empty, 1 );
        if( ( *mark ) == ':' ) {
            ( *mark ) = '\0';
            if( http_header_field_check_scheme( data, scheme ) ) {
                length = http_header_field_value_length( mark + 1 );
                store = ( uint8_t* ) os_malloc( length + 1 );
                end = http_header_field_parse_value( mark + 1, store, 0 );

                if( strcmp( content_length, ( char* ) data ) == 0 ) {
                    parse_uint32( &( request->content_length ), store );
                } else if( strcmp( host, ( char* ) data ) == 0 ) {
                    request->location->hostname = store;
                } else {
                    request->headers = http_header_field_add( request->headers, data, store );
                    os_free( store );
                }
                data = end;
            } else {
                data = http_header_field_parse_value( mark + 1, ( uint8_t* ) empty, 1 );
            }
            ( *mark ) = ':';
        } else break;
    }

    if(( *data ) == '\r' && ( *( data + 1 ) == '\n' )) {
        data += 2;
        if( request->content_length > 0 )
            url_parse_query( request->location, data, ( int16_t ) request->content_length );
        request->content = (( *data ) == '\0' ) ? NULL : data;
    } else request->content = data;

	if( scheme == ( HTTP_REQUEST_SCHEME | WS_REQUEST_SCHEME ) ) {
		if( http_header_field_get( request->headers, ( uint8_t* ) header_sec_ws_key ) != NULL ) request->location->protocol = URL_PROTOCOL_WS;
		else request->location->protocol = URL_PROTOCOL_HTTP;
	} else if( scheme == WS_REQUEST_SCHEME ) request->location->protocol = URL_PROTOCOL_WS;
    else request->location->protocol = URL_PROTOCOL_HTTP;

    return data;
}


#endif
