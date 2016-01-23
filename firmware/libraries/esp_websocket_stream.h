/**
 * \brief		RFC6455 WebSocket Stream implementation
 * \file		websocket_stream.h
 * \author		Cristian Dobre
 * \version 	0.0.1
 * \date 		April 2015
 * \copyright 	Revised BSD License.
 *
 * WebSocket protocol implementation for high speed, low latency data transfer. Includes optimized parsers and packet
 * generator designed with low requirements for highly embedded applications.
 *
 * To use, register a callback for the received packets and feed the incoming data into the parser.
 * A random number generator function should be defined. If not, the library uses a stdlib and time.h random
 * implementation, but it may not be available on all platforms.
 */

#ifndef __RFC6455_WEBSOCKET_STREAM_H__
#define __RFC6455_WEBSOCKET_STREAM_H__

/**
 * /def 		WEBSOCKET_PROPRIETARY_RANDOM_IMPLEMENTATION
 * /brief		Define this before including the library to use your own random number generator.
 * /see 		__ws_stream_generate_mask
 */
//#define WEBSOCKET_PROPRIETARY_RANDOM_IMPLEMENTATION ;

/**
 * BIT OPERATION MACROS
 */
#define	__WS_BIT_SET( TARGET, MASK )		( (TARGET) |= (MASK) )
#define	__WS_BIT_CLEAR( TARGET, MASK )		( (TARGET) &= ~(MASK) )
#define __WS_BIT_CHECK( TARGET, MASK )		( ( (TARGET) & (MASK) ) == (MASK) )

/**
 * CONTROL BITS
 */
#define __WS_FIN_BIT		((uint8_t) 0b10000000)
#define __WS_MASK_BIT		((uint8_t) 0b10000000)

/**
 * MASK FOR MULTIPLE BIT VALUES
 */
#define __WS_MASK_RSV_BITS		((uint8_t) 0b01110000)
#define __WS_MASK_OPCODE_BITS	((uint8_t) 0b00001111)
#define __WS_MASK_LENGTH_BITS	((uint8_t) 0b01111111)


/**
 * /def 		WEBSOCKET_STREAM_DATA_TEXT
 * /brief		Code value for packet containing text data
 */
#define WEBSOCKET_STREAM_DATA_TEXT		0x1

/**
 * /def 		WEBSOCKET_STREAM_DATA_BINARY
 * /brief		Code value for packet containing binary data
 */
#define WEBSOCKET_STREAM_DATA_BINARY	0x2

typedef enum {
	__WS_OPCODE_RESERVED		= 0xF,
	__WS_OPCODE_CONTINUATION 	= 0x0,
	__WS_OPCODE_TEXT			= WEBSOCKET_STREAM_DATA_BINARY,
	__WS_OPCODE_BINARY			= WEBSOCKET_STREAM_DATA_TEXT,
	__WS_OPCODE_CLOSE			= 0x8,
	__WS_OPCODE_PING			= 0x9,
	__WS_OPCODE_PONG			= 0xA
} __ws_packet_opcode_t;


/**
 * FINITE STATE PARSER
 */
typedef enum {
	__WS_PARSE_IDLE 			= 0,
	__WS_PARSE_HEADER,
	__WS_PARSE_MASK,
	__WS_PARSE_DATA,
}  __ws_stream_parser_state_t;


/**
 * WEBSOCKET STREAM DECODE CONTEXT
 * Contains data related to the stream and parser states
 */
typedef struct __ws_stream_decode_context {
	__ws_stream_parser_state_t parser_state;
	uint8_t *data_buffer;
	void ( *received_callback )( uint8_t, uint8_t *, uint32_t );
	uint8_t has_mask;
	uint8_t opcode;
	uint8_t data_mask[ 4 ];
	uint8_t header_size;
	uint32_t packet_size;
	uint32_t data_size;
	uint8_t *packet_size_p;
	uint8_t *payload_data;
} websocket_stream_decode_context;


/**
 * /fn			__ws_stream_generate_mask
 * /brief		Generate a random 32 bit integer value for data masking
 * /return 		uint32_t
 * /see 		WEBSOCKET_PROPRIETARY_RANDOM_IMPLEMENTATION
 *
 * Define the WEBSOCKET_PROPRIETARY_RANDOM_IMPLEMENTATION value before including this library, and implement this
 * function to use the available number generator. If the define doesn't exist, a stdlib and time.h random number
 * generator is used, but it may not be available on all platforms.
 */
uint32_t __ws_stream_generate_mask( void );


/**
 * /fn          websocket_stream_encode
 * /brief       Create a WebSocket packet from the given data
 */
uint32_t websocket_stream_encode( uint8_t *dest_data, uint8_t* source_data, uint32_t data_length, uint8_t opcode, uint8_t mask_data );


/**
 * /fn 			websocket_stream_decode_init
 * /brief		Initialize the stream decoder and register the message callback
 */
void websocket_stream_decode_init( websocket_stream_decode_context *stream_context, uint8_t *message_buffer, void ( *fn )( uint8_t, uint8_t *, uint32_t ) );

/**
 * /fn 			websocket_stream_decode
 * /brief		Decodes the given data
 */
void websocket_stream_decode( websocket_stream_decode_context *stream_context, uint8_t *data, uint32_t length );

/**
 * /fn 			websocket_stream_decode_one
 * /brief		Feeds one more character to the parser
 */
void websocket_stream_decode_one( websocket_stream_decode_context *stream_context, uint8_t c );

/**
 * /fn 			__ws_stream_parser_valid_opcode
 * /brief		Checks if the given opcode is valid
 */
uint8_t __ws_stream_parser_valid_opcode( uint8_t b );

/**
 * /fn 			__ws_stream_parser_byte_is_header_start
 * /brief		Checks if a received byte might be the starting byte in a packet
 * Relies on the values of the reserved bits and the correspondence between FIN bit and op-code. Feeding bad data might
 * confuse the parser, making it lose several packets in a row.
 */
uint8_t __ws_stream_parser_byte_is_header_start( uint8_t b );


#endif