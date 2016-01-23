/**
 * \brief		RFC6455 WebSocket Stream implementation
 * \file		websocket_stream.c
 * \author		Cristian Dobre
 * \version 	0.0.1
 * \date 		April 2015
 * \copyright 	Revised BSD License.
 */
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "websocket_stream.h"

#ifndef WEBSOCKET_PROPRIETARY_RANDOM_IMPLEMENTATION
#include <time.h>
#include <stdlib.h>

uint32_t __ws_stream_generate_mask( void )
{
	static uint8_t __ws_stdlib_rand_seed = 0;
	uint32_t r = 0;
	uint16_t *p = (uint16_t *) &r;

	if( __ws_stdlib_rand_seed == 0 ){
		__ws_stdlib_rand_seed = 1;
		srand( time( NULL ) );
	}

	*( p ) = rand();
	*( p + 1 ) = rand();
	return r;
}
#endif




uint32_t websocket_stream_encode( uint8_t* dest_data, uint8_t* source_data, uint32_t data_length, uint8_t opcode, uint8_t mask_data )
{
    uint8_t* packet_data;
    uint8_t header_size = 2;
    uint32_t data_index = 0;
    uint32_t data_mask, i;
    uint8_t* data_mask_p = (uint8_t*) &data_mask;

    if( mask_data ) {
        header_size += 4;
        data_mask = __ws_stream_generate_mask();
    }
    dest_data[ 0 ] = dest_data[ 1 ] = 0x00;
    // add up the length
    if( data_length < 0x7E ) ;
    else if( data_length < 0xFFFF ) header_size += 2;
    else header_size += 8;
    __WS_BIT_SET( dest_data[ 0 ], __WS_FIN_BIT );
    // set opcode
    dest_data[ 0 ] |= opcode & __WS_MASK_OPCODE_BITS;
    // set the mask bit
    if( mask_data ) __WS_BIT_SET( dest_data[ 1 ], __WS_MASK_BIT );
    // set length and mask if required
    if( data_length < 0x7E ) {
        dest_data[ 1 ] |= __WS_MASK_LENGTH_BITS & data_length;
        if( mask_data ) *(( uint32_t * )( dest_data + 2 )) = data_mask;
    } else if( data_length < 0xFFFF ){
        dest_data[ 1 ] |= 0x7E;
        *(( uint8_t * )( dest_data + 2 )) = ( data_length & 0xFF00 ) >> 8;
        *(( uint8_t * )( dest_data + 3 )) = ( data_length & 0x00FF );
        if( mask_data ) *(( uint32_t * )( dest_data + 4 )) = data_mask;
    } else{
        dest_data[ 1 ] |= 0x7F;
        dest_data[ 2 ] = dest_data[ 3 ] = dest_data[ 4 ] = dest_data[ 5 ] = 0x00;
        *(( uint8_t * )( dest_data + 6 )) = ( data_length & 0xFF000000UL ) >> 24;
        *(( uint8_t * )( dest_data + 7 )) = ( data_length & 0x00FF0000UL ) >> 16;
        *(( uint8_t * )( dest_data + 8 )) = ( data_length & 0x0000FF00UL ) >> 8;
        *(( uint8_t * )( dest_data + 9 )) = ( data_length & 0x000000FFUL );
        if( mask_data ) *(( uint32_t * )( dest_data + 10 )) = data_mask;
    }
    packet_data = dest_data + header_size;
    // copy the data into the packet
    if( mask_data )
        for( ; data_index < data_length ; data_index++, packet_data++ )
             (*packet_data) = source_data[ data_index ] ^ data_mask_p[ data_index % 4 ];
    else
        for( ; data_index < data_length ; data_index++, packet_data++ )
            (*packet_data) = source_data[ data_index ];
    // return the total packet length
    return data_length + header_size;
}


void websocket_stream_decode_init( websocket_stream_decode_context *stream_context, uint8_t *message_buffer, void ( *fn )( uint8_t, uint8_t *, uint32_t ) )
{
	// initialize the parser context
	stream_context -> data_buffer = message_buffer;
	stream_context -> parser_state = __WS_PARSE_IDLE;
	stream_context -> received_callback = fn;
	// and parser variables
	stream_context -> has_mask = stream_context -> opcode = stream_context -> header_size = 0;
	stream_context -> packet_size = stream_context -> data_size = 0;
	stream_context -> packet_size_p = (uint8_t *) &( stream_context ->packet_size );
}


void websocket_stream_decode_one( websocket_stream_decode_context *stream_context, uint8_t b )
{
	// check to see if we are receiving a new packet
	if( stream_context -> parser_state == __WS_PARSE_IDLE && __ws_stream_parser_byte_is_header_start( b ) ){
		// change state to header parse
		stream_context -> parser_state = __WS_PARSE_HEADER;
		// set variables
		stream_context -> packet_size_p = (uint8_t *) ( &( stream_context -> packet_size ) );
		stream_context -> opcode = b & __WS_MASK_OPCODE_BITS;
		stream_context -> header_size = 14;
		stream_context -> packet_size = 0;
    } else if( stream_context -> parser_state == __WS_PARSE_HEADER ) {
		// parse second byte of header
		if( stream_context -> header_size == 14 ){
			// set variables
			stream_context -> has_mask = __WS_BIT_CHECK( b, __WS_MASK_BIT );
			stream_context -> packet_size = b & __WS_MASK_LENGTH_BITS;
			// calculate header length
			if( stream_context -> packet_size == 0x7E ) stream_context -> header_size = 2;
			else if( stream_context -> packet_size == 0x7F ) stream_context -> header_size = 8;
			else {
				stream_context -> header_size = 0;
				// jump to the next state
				if( stream_context -> has_mask ){
					stream_context -> parser_state = __WS_PARSE_MASK;
					stream_context -> header_size = 4;
				} else {
					stream_context -> parser_state = __WS_PARSE_DATA;
					stream_context -> data_size = 0;
				}
			}
		// parse remaining header_size
		} else {
			stream_context -> header_size--;
			// save the header size
			if( stream_context -> header_size > 3 ) ;
			else if( stream_context -> header_size == 1 ) stream_context -> packet_size_p[ 1 ] = b;
			else if( stream_context -> header_size == 0 ){
				stream_context -> packet_size_p[ 0 ] = b;
				// jump to the next state
				if( stream_context -> has_mask ){
					stream_context -> parser_state = __WS_PARSE_MASK;
					stream_context -> header_size = 4;
				} else {
					stream_context -> parser_state = __WS_PARSE_DATA;
					stream_context -> data_size = 0;
				}
			} else if( stream_context -> header_size == 3 ) stream_context -> packet_size_p[ 2 ] = b;
			else if( stream_context -> header_size == 2 ) stream_context -> packet_size_p[ 3 ] = b;
		}
	// parse masking key
	} else if( stream_context -> parser_state == __WS_PARSE_MASK ) {
	    stream_context -> header_size --;
		( stream_context -> data_mask )[ 3 - ( stream_context -> header_size ) ] = b;
		if( stream_context -> header_size == 0 ) {
			stream_context -> parser_state = __WS_PARSE_DATA;
			stream_context -> data_size = 0;
		}
	// parse and unmask payload data
	} else if( stream_context -> parser_state == __WS_PARSE_DATA ) {
		stream_context -> data_buffer[ stream_context -> data_size ] = b;
		// packet received, run the callback
		if( ++( stream_context -> data_size ) == stream_context -> packet_size ) {
			stream_context -> data_size = 0;
			// unmask the data
			if( stream_context -> has_mask )
				while( stream_context -> data_size != stream_context -> packet_size ){
					stream_context -> data_buffer[ stream_context -> data_size ] ^= ( stream_context -> data_mask )[ stream_context -> data_size % 4 ];
					( stream_context -> data_size ) ++;
				}
			// run the callback function to parse the data
			stream_context -> received_callback( stream_context -> opcode, stream_context -> data_buffer, stream_context -> packet_size );
			// return to idle state
			stream_context -> parser_state = __WS_PARSE_IDLE;
		}
	}
}


void websocket_stream_decode( websocket_stream_decode_context *stream_context, uint8_t *data, uint32_t length )
{
	while( length -- ){
		websocket_stream_decode_one( stream_context, *( data++ ) );
	}
}


uint8_t __ws_stream_parser_valid_opcode( uint8_t b )
{
	switch( b ){
		case __WS_OPCODE_CONTINUATION:
			return 1;
		case __WS_OPCODE_TEXT:
			return 2;
		case __WS_OPCODE_BINARY:
			return 2;
		case __WS_OPCODE_CLOSE:
			return 3;
		case __WS_OPCODE_PING:
			return 3;
		case __WS_OPCODE_PONG:
			return 3;
		default:
			return 0;
	}
	return 0;
}


uint8_t __ws_stream_parser_byte_is_header_start( uint8_t b )
{
	if( ( b & __WS_MASK_RSV_BITS ) != 0x00 ) return 0;
	if( __WS_BIT_CHECK( b, __WS_FIN_BIT ) ) {
		// reserved opcode
		if( __ws_stream_parser_valid_opcode( b & __WS_MASK_OPCODE_BITS ) == 0 ) return 0;
	} else {
		// reserved and continuation opcodes
		if( __ws_stream_parser_valid_opcode( b & __WS_MASK_OPCODE_BITS ) < 2 ) return 0;
	}
	return 1;
}
