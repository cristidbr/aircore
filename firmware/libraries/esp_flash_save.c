/**
 * \brief		ESP8266 Flash parameter storage
 * \file		esp_flash_save.c
 * \author		Cristian Dobre
 * \version 	1.0.0
 * \date 		August 2015
 * \copyright 	Revised BSD License.
 */
#ifndef __ESP_WS_WIFI_ACCESS_C__
#define __ESP_WS_WIFI_ACCESS_C__


#include "esp_flash_save.h"

#include "user_interface.h"
#include "mem.h"

LOCAL uint32_t esp_flash_current_sector_address = 0x0003c000;
LOCAL uint32_t esp_flash_backup_sector_address = 0x0003d000;




/**
 * Queue actions
 */
typedef enum esp_flash_queue_actions {
	ESP_FLASH_QUEUE_NONE = 0x00,
	ESP_FLASH_QUEUE_REMOVE = 0x01,
	ESP_FLASH_QUEUE_SAVE = 0x02
} esp_flash_queue_actions_type;

/**
 * Queued update items
 */
typedef struct esp_flash_update_item_queue {
	uint8_t item_action;
	uint8_t item_id;
	uint8_t *item_data;
	uint8_t item_size;
} esp_flash_update_item_queue_type;

/**
 * Update queue
 */
LOCAL uint8_t esp_flash_instant_update = 0x01;
LOCAL esp_flash_update_item_queue_type esp_flash_update_queue[ ESP_FLASH_UPDATE_QUEUE_SIZE ];
LOCAL uint8_t esp_flash_update_queue_fill = 0;




/**
 * Calculate the checksum for the given data
 */
LOCAL uint32_t ICACHE_FLASH_ATTR esp_flash_compute_checksum( uint32_t* sector_data )
{
    uint32_t i, checksum = 0x00000000;
    for( i = 0; i < ESP_FLASH_SECTOR_LENGTH - 1 ; i++ )
        checksum ^= sector_data[ i ];
    return checksum;
}



/**
 * Verify the data checksum
 */
uint8_t ICACHE_FLASH_ATTR esp_flash_verify_checksum( uint32_t* sector_data )
{
    if( sector_data[ ESP_FLASH_SECTOR_LENGTH - 1 ] == esp_flash_compute_checksum( sector_data ) )
        return 0x01;
    return 0x00;
}



/**
 * Iterate and find parameter
 */
LOCAL uint32_t* ICACHE_FLASH_ATTR esp_flash_search_parameter( uint8_t parameter_id, uint32_t* segment_start, uint32_t* segment_end )
{
    uint8_t* read_addr = ( uint8_t* ) segment_start;
    uint8_t data_size = 0x00;
    // iterate within data in the segment
    while( segment_start <= segment_end )
    {
        read_addr = ( uint8_t* ) segment_start;
        // end of list
        if( *( read_addr ) == 0x00 )
            break;
        data_size = read_addr[ 1 ];
        // found parameter
        if( *( read_addr ) == parameter_id ){
            return ( uint32_t* ) read_addr;
        }
        // jump to next read address
        data_size = ( ( 2 + data_size + 0x3 ) & ( ~0x3 ) ) / sizeof( uint32_t );
        segment_start += data_size;
    }
    return NULL;
}

/**
 * Update flash data
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_data_save( uint32_t* data )
{
    uint16_t* wear_level = ( uint16_t* ) data;
    uint16_t current_wear_level = *( wear_level );
    // swap sectors
    uint32_t flash_swap = esp_flash_backup_sector_address;
    esp_flash_backup_sector_address = esp_flash_current_sector_address;
    esp_flash_current_sector_address = flash_swap;
    // increase wear level
    *( wear_level ) = current_wear_level + 1;
    // update checksum
    data[ ESP_FLASH_SECTOR_LENGTH - 1 ] = esp_flash_compute_checksum( data );
    // erase and save to flash
    spi_flash_erase_sector( esp_flash_current_sector_address >> 12 );
    spi_flash_write( esp_flash_current_sector_address, ( uint32* ) data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) );
}




/**

 * Get address of segment end for static parameters

 */
LOCAL uint32_t* ICACHE_FLASH_ATTR esp_flash_get_segment_end( uint32_t* segment_start )
{
    uint8_t* parameter_start = ( uint8_t* ) segment_start;
    uint8_t data_size;
    // advance to next parameter
    while( parameter_start[ 0 ] != 0x00 ){
        data_size = parameter_start[ 1 ];
        parameter_start += (( 2 + data_size + 0x3 ) & ( ~0x03 ));
    }
    return ( uint32_t* ) parameter_start;
}



/**
 * Search for an insert location in the given segment
 */
LOCAL uint32_t* ICACHE_FLASH_ATTR esp_flash_get_insert_address( uint32_t* segment_address, uint8_t parameter_id )
{
    uint8_t* segment_start = ( uint8_t* ) segment_address;
    uint8_t* parameter_start = segment_start;
    // search for a greater id value
    while( parameter_start[ 0 ] != 0x00 && parameter_start[ 1 ] != 0x00 ){
        if( parameter_start[ 0 ] > parameter_id )
            break;
        parameter_start += (( 2 + parameter_start[ 1 ] + 0x3 ) & ( ~0x03 ));
    }
    return ( uint32_t* ) parameter_start;        
}


/**
 * Shifts the array contents and inserts the new parameter
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_parameter_insert_shift( uint8_t* segment_insert, uint8_t* segment_end, uint8_t* parameter, uint16_t parameter_size )
{
    uint8_t* copy_end = segment_end + parameter_size;
    uint16_t i, copy_length;
    // shift data
    while( segment_end > segment_insert ){
        copy_end --;
        segment_end --;
        *( copy_end ) = *( segment_end );
    }
    // copy the new parameter
    os_memcpy( segment_insert, parameter, parameter_size );
}


/**

 * Insert static parameter
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_insert_parameter( uint32_t* sector, uint8_t parameter_id, uint8_t* data_src, uint8_t data_size )
{
    uint8_t* parameter;
    uint16_t insert_size = (( 2 + data_size + 0x3 ) & ( ~0x03 ));
    uint8_t* segment_start = ( uint8_t* ) ( sector + ESP_FLASH_SECTOR_HEADER_SIZE );
    uint8_t* segment_end = ( uint8_t* ) esp_flash_get_segment_end( ( uint32_t* ) segment_start );
    uint8_t* parameter_insert_start = ( uint8_t* ) esp_flash_get_insert_address( ( uint32_t* ) segment_start, parameter_id );
    // create parameter 
    parameter = ( uint8_t* ) os_zalloc( insert_size );
    parameter[ 0 ] = parameter_id;
    parameter[ 1 ] = data_size;
    os_memcpy( parameter + 2, data_src, data_size );
    // insert parameter
    esp_flash_parameter_insert_shift( parameter_insert_start, segment_end, parameter, insert_size );
    // free memory
    os_free( parameter );
}




/**
 * Removes static parameter from sector
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_delete_parameter( uint32_t* parameter_addr, uint32_t* data )
{
    uint8_t* parameter_start = ( uint8_t* ) parameter_addr;
    uint8_t* parameter_end;
    uint8_t* segment_end;
    uint8_t* data_start = ( uint8_t* ) ( data + ESP_FLASH_SECTOR_HEADER_SIZE );
    uint8_t remove_size = (( 2 + parameter_start[ 1 ] + 0x3 ) & ( ~0x03 ));
    // compute parameter end address
    parameter_end = (( uint8_t* ) parameter_addr ) + remove_size;
    // get segment end address
    segment_end = ( uint8_t* ) esp_flash_get_segment_end( data + ESP_FLASH_SECTOR_HEADER_SIZE );
    // overwrite removed parameter
    while( parameter_end < segment_end ){
        *( parameter_start ) = *( parameter_end );
        parameter_start ++;
        parameter_end ++;
    }
    // zerofill by enough bytes
    os_memset( parameter_start, 0x00, remove_size );    
}


/**
 * Updates the sector data by saving a parameter
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_save_parameter_update( uint32_t* sector_data, uint8_t parameter_id, uint8_t* data_src, uint8_t data_size )
{
	uint8_t* segment_start = ( uint8_t* ) ( sector_data + ESP_FLASH_SECTOR_HEADER_SIZE );
	uint32_t* data_found = NULL;
	uint32_t* data_start;
	uint32_t* data_end = sector_data + ESP_FLASH_SECTOR_LENGTH - 1;
	// iterate and find parameter
	data_start = sector_data + ESP_FLASH_SECTOR_HEADER_SIZE;
	data_found = esp_flash_search_parameter( parameter_id, data_start, data_end );
	// remove the parameter if exists
	if( data_found != NULL ) {
		esp_flash_delete_parameter( data_found, sector_data );
	}
	// insert the parameter
	esp_flash_insert_parameter( sector_data, parameter_id, data_src, data_size );
}





/**
 * Updates sector data by removing the parameter, does not save the update to flash
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_remove_parameter_update( uint32_t* sector_data, uint8_t parameter_id )
{
	uint8_t* segment_start = ( uint8_t* ) ( sector_data + ESP_FLASH_SECTOR_HEADER_SIZE );
	uint32_t* data_found = NULL;
	uint32_t* data_start;
	uint32_t* data_end = sector_data + ESP_FLASH_SECTOR_LENGTH - 1;
	
	// iterate and find parameter
        data_start = sector_data + ESP_FLASH_SECTOR_HEADER_SIZE;
        data_found = esp_flash_search_parameter( parameter_id, data_start, data_end );
        // remove the parameter if exists
        if( data_found != NULL ) {
            esp_flash_delete_parameter( data_found, sector_data );
        }
}


/**
 * Runs the queued actions and updates the flash 
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_queue_run( void )
{
	uint32_t data[ ESP_FLASH_SECTOR_LENGTH ];	
	uint8_t i;
	// check for items in the queue	
	if( esp_flash_update_queue_fill != 0 ){
		// read current sector data
		if( SPI_FLASH_RESULT_OK == spi_flash_read( esp_flash_current_sector_address, ( uint32* ) data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) ){	
			// runs the actions
			for( i  = 0 ; i < esp_flash_update_queue_fill ; i ++ ){
				// remove the parameter		
				if( esp_flash_update_queue[ i ].item_action == ESP_FLASH_QUEUE_REMOVE ){
					esp_flash_remove_parameter_update( data, esp_flash_update_queue[ i ].item_id );
				} else if( esp_flash_update_queue[ i ].item_action == ESP_FLASH_QUEUE_SAVE ){
					// save parameter			
					esp_flash_save_parameter_update( data, esp_flash_update_queue[ i ].item_id,  esp_flash_update_queue[ i ].item_data, esp_flash_update_queue[ i ].item_size );
					// free memory
					os_free( esp_flash_update_queue[ i ].item_data );
				}
			}
			// apply the updates to flash
			esp_flash_data_save( data );
			// reset fill counter
			esp_flash_update_queue_fill = 0;
		}
	}
}

/**
 * Enables instant update
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_enable_instant_update( void )
{
	esp_flash_instant_update = 0x01;
	esp_flash_queue_run();
}

/**
 * Disables instant update
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_disable_instant_update( void )
{
	esp_flash_instant_update = 0x00;
}

/**
 * Adds a remove action in the queue
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_queue_remove_parameter( uint8_t parameter_id )
{
	// check if the queue is full	
	if( esp_flash_update_queue_fill == ESP_FLASH_UPDATE_QUEUE_SIZE )
		esp_flash_queue_run();
	// save into queue
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_action = ESP_FLASH_QUEUE_REMOVE;
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_id = parameter_id;
	esp_flash_update_queue_fill ++;
}


/**
 * Add a save action in the queue
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_queue_save_parameter( uint8_t parameter_id, uint8_t* data_src, uint8_t data_size )
{
	// check if the queue is full	
	if( esp_flash_update_queue_fill == ESP_FLASH_UPDATE_QUEUE_SIZE )
		esp_flash_queue_run();
	// save into queue
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_action = ESP_FLASH_QUEUE_SAVE;
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_id = parameter_id;
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_size = data_size;
	esp_flash_update_queue[ esp_flash_update_queue_fill ].item_data = ( uint8_t* ) os_malloc( data_size );
	// copy data 
	os_memcpy( esp_flash_update_queue[ esp_flash_update_queue_fill ].item_data, data_src, data_size );	
	esp_flash_update_queue_fill ++;
}


/**
 * Search in the flash queue for parameters
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_queue_search( uint8_t parameter_id )
{
	uint8_t i = esp_flash_update_queue_fill;
	// nothing found in the queue	
	if( esp_flash_update_queue_fill == 0 )
		return ESP_FLASH_QUEUE_NONE;
	// search the queue
	for( i = esp_flash_update_queue_fill - 1 ; i >= 0 ; i -- ){
		if( esp_flash_update_queue[ i ].item_id == parameter_id )
			return esp_flash_update_queue[ i ].item_action;
	}
	return ESP_FLASH_QUEUE_NONE;
}

/**
 * Retrieve the queued value for a parameter
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_queue_read_value( uint8_t parameter_id, uint8_t* data_src )
{
	uint8_t i;
	// nothing found in the queue	
	if( esp_flash_update_queue_fill == 0 ) 
		return 0;
	// search the queue
	for( i = esp_flash_update_queue_fill - 1 ; i >= 0 ; i -- ){
		if( esp_flash_update_queue[ i ].item_id == parameter_id ){
			os_memcpy( data_src, esp_flash_update_queue[ i ].item_data, esp_flash_update_queue[ i ].item_size );
			return esp_flash_update_queue[ i ].item_size;
		}
	}
	return 0x00;	
}








/**
 * Initialize primary sector into flash
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_initialize_sector( uint32_t sector_address )
{
    uint32_t sector_data[ ESP_FLASH_SECTOR_LENGTH ];
    uint32_t config_flags = ESP_FLASH_DEFAULT_CONFIG_FLAGS;
    uint16_t wear_level = 0x0001;
    // zero all data
    os_memset( ( uint8_t* ) sector_data, 0x00, ( uint16_t )( ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) );   
    // copy header data
    os_memcpy( ( uint16_t* ) sector_data, ( uint16_t* ) &wear_level, sizeof( wear_level ) / sizeof( uint16_t ) );
    os_memcpy( ( uint8_t* )( sector_data + 1 ), &config_flags, sizeof( config_flags ) );
    // seal with checksum
    sector_data[ ESP_FLASH_SECTOR_LENGTH - 1 ] = esp_flash_compute_checksum( sector_data );
    // erase and flash
    spi_flash_erase_sector( sector_address >> 12 );
    spi_flash_write( sector_address, ( uint32* ) sector_data, ( uint16_t )( ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) );
}



/**
 * Checks flash data integrity
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_verify_data( void )
{
    uint8_t flash_fail = 0x01;
    uint16_t primary_order = 0x0000, secondary_order = 0x0000;
    uint32_t sector_data[ ESP_FLASH_SECTOR_LENGTH ];
    uint16_t* data_pnt = ( uint16_t* ) sector_data;
    // check primary sector
    if( SPI_FLASH_RESULT_OK == spi_flash_read( ESP_FLASH_PRIMARY_SECTOR_ADDRESS, ( uint32* ) sector_data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) ){
        if( ! esp_flash_verify_checksum( sector_data ) ) {
            flash_fail = 0x00;
        } else {
            primary_order = *( data_pnt );
        }
    } else {
        flash_fail = 0x00;
    }
    // check secondary sector
    if( SPI_FLASH_RESULT_OK == spi_flash_read( ESP_FLASH_SECONDARY_SECTOR_ADDRESS, ( uint32* ) sector_data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) ){
        if( ! esp_flash_verify_checksum( sector_data ) ) {
            flash_fail = 0x00;
        } else {
            secondary_order = *( data_pnt );
        }
    } else {
        flash_fail = 0x00;
    }
    // save primary and secondary sectors
    if( primary_order != 0x0000 && primary_order != 0xFFFF ){
        if( secondary_order != 0x0000 && secondary_order != 0xFFFF ){
            if( primary_order > secondary_order ) {
                esp_flash_current_sector_address = ESP_FLASH_PRIMARY_SECTOR_ADDRESS;
                esp_flash_backup_sector_address = ESP_FLASH_SECONDARY_SECTOR_ADDRESS;
            } else {
                esp_flash_current_sector_address = ESP_FLASH_SECONDARY_SECTOR_ADDRESS;
                esp_flash_backup_sector_address = ESP_FLASH_PRIMARY_SECTOR_ADDRESS;
            }                
        } else {
            esp_flash_current_sector_address = ESP_FLASH_PRIMARY_SECTOR_ADDRESS;
            // corrupted, will repair at next parameter update
            esp_flash_backup_sector_address = ESP_FLASH_SECONDARY_SECTOR_ADDRESS;
        }
    } else if( secondary_order != 0x0000 && secondary_order != 0xFFFF ){
        esp_flash_current_sector_address = ESP_FLASH_SECONDARY_SECTOR_ADDRESS;
        // corrupted
        esp_flash_backup_sector_address = ESP_FLASH_PRIMARY_SECTOR_ADDRESS;
    } else {
        // both uninitialized or corrupted, initialize primary
        esp_flash_current_sector_address = ESP_FLASH_PRIMARY_SECTOR_ADDRESS;
        esp_flash_backup_sector_address = ESP_FLASH_SECONDARY_SECTOR_ADDRESS;
        esp_flash_initialize_sector( esp_flash_current_sector_address );
    }
    return flash_fail;
}



/**
 * Find parameter and it's value, returns parameter length
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_read_parameter( uint8_t parameter_id, uint8_t* dst )
{
	uint8_t* read_addr;
	uint8_t data_size = 0x00, parameter_size;
	uint32_t sector_data[ ESP_FLASH_SECTOR_LENGTH ];
	uint32_t* data_start;
	uint32_t* data_end = sector_data + ESP_FLASH_SECTOR_LENGTH - 1;
	uint32_t* data_found = NULL;
	// read current sector data
	if( SPI_FLASH_RESULT_OK != spi_flash_read( esp_flash_current_sector_address, ( uint32* ) sector_data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) )
		return data_size;
	// check queue for parameters	
	if( esp_flash_instant_update == 0x00 ){
		data_size = esp_flash_queue_search( parameter_id );
		// check if the parameter was removed
		if( data_size == ESP_FLASH_QUEUE_REMOVE ){
			return 0x00;
		} else if( data_size == ESP_FLASH_QUEUE_SAVE ){
			// copy the most recent value from the queue			
			data_size = esp_flash_queue_read_value( parameter_id, dst );
			return data_size;
		} 
	}
	// iterate and find parameter
	data_start = sector_data + ESP_FLASH_SECTOR_HEADER_SIZE;
	data_found = esp_flash_search_parameter( parameter_id, data_start, data_end );
	// check if parameter exists 
	if( data_found != NULL ) {
		read_addr = ( uint8_t* ) data_found;
		data_size = read_addr[ 1 ];
		// copy data into the given memory space
		os_memcpy( dst, read_addr + 2, data_size );
	}
	return data_size;
}





/**
 * Save parameter
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_save_parameter( uint8_t parameter_id, uint8_t* data_src, uint8_t data_size )
{
	uint32_t sector_data[ ESP_FLASH_SECTOR_LENGTH ];
	// check if instant update is enabled
	if( esp_flash_instant_update == 0x00 ){
		// add parameter into queue
		esp_flash_queue_save_parameter( parameter_id, data_src, data_size );
	} else {
		// read current sector data
		if( SPI_FLASH_RESULT_OK == spi_flash_read( esp_flash_current_sector_address, ( uint32* ) sector_data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) ){
			// update the parameter
			esp_flash_save_parameter_update( sector_data, parameter_id, data_src, data_size );
			// write to flash
			esp_flash_data_save( sector_data );
		}
	}
}


/**
 * Clean the parameter and save to flash
 */
LOCAL void ICACHE_FLASH_ATTR esp_flash_remove_parameter( uint8_t parameter_id )
{
    uint32_t sector_data[ ESP_FLASH_SECTOR_LENGTH ];
    
	// check if instant updates are enabled
	if( esp_flash_instant_update == 0x00 ){
		// save the action into queue
		esp_flash_queue_remove_parameter( parameter_id );
	} else {
		// read current sector data
		if( SPI_FLASH_RESULT_OK == spi_flash_read( esp_flash_current_sector_address, ( uint32* ) sector_data, ESP_FLASH_SECTOR_LENGTH * sizeof( uint32_t ) ) ){
			// remove parameter
			esp_flash_remove_parameter_update( sector_data, parameter_id );
			// write to flash
			esp_flash_data_save( sector_data );       
		}
	}
}



/**
 * Initialize SPI flash for storing parameter data
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_setup( void )
{
    esp_flash_verify_data();
}


#endif

