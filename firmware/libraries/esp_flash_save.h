/**
 * \brief		ESP8266 Flash parameter storage
 * \file		esp_flash_save.h
 * \author		Cristian Dobre
 * \version 	1.0.0
 * \date 		August 2015
 * \copyright 	Revised BSD License.
 */
#ifndef __ESP_FLASH_PARAMETER_SAVE__
#define __ESP_FLASH_PARAMETER_SAVE__

#include "user_interface.h"

#include "osapi.h"

#define ESP_FLASH_ALIGN __declspec(align(4))

#define ESP_FLASH_SECTOR_HEADER_SIZE 2
#define ESP_FLASH_SECTOR_LENGTH 1024
#define ESP_FLASH_PRIMARY_SECTOR_ADDRESS 0x0003C000
#define ESP_FLASH_SECONDARY_SECTOR_ADDRESS 0x0003D000

#define ESP_FLASH_DEFAULT_CONFIG_FLAGS 0x00000000

#define ESP_FLASH_UPDATE_QUEUE_SIZE 16

LOCAL uint32_t esp_flash_current_sector_address;
LOCAL uint32_t esp_flash_backup_sector_address;


/**
 * API
 */
LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_setup( void );

LOCAL uint8_t ICACHE_FLASH_ATTR esp_flash_read_parameter( uint8_t parameter_id, uint8_t* dst );
LOCAL void ICACHE_FLASH_ATTR esp_flash_remove_parameter( uint8_t parameter_id );
LOCAL void ICACHE_FLASH_ATTR esp_flash_save_parameter( uint8_t parameter_id, uint8_t* data_src, uint8_t data_size );

LOCAL void ICACHE_FLASH_ATTR esp_flash_enable_instant_update( void );
LOCAL void ICACHE_FLASH_ATTR esp_flash_disable_instant_update( void );


#endif
