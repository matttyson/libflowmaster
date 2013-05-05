#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "flowmaster_private.h"
#include "protocol.h"
#include "bootloader_protocol.h"

static uint8_t flash_char_to_nibble(uint8_t byte);
static uint8_t flash_convert_byte(uint8_t high, uint8_t low);
static uint16_t flash_convert_word(uint8_t hh, uint8_t hl, uint8_t lh, uint8_t ll);
static uint8_t flash_read_length(const char *buffer);
static uint16_t flash_read_address(const char *buffer);
static uint8_t flash_read_type(const char *buffer);
static uint8_t flash_read_bytes(const char *src, uint8_t *dest, int destlen);
static uint8_t flash_read_checksum(const char *buffer);
static uint8_t flash_calc_checksum(uint8_t size, uint16_t address, uint8_t type, uint8_t *data);
static void flash_show_program_message(flowmaster *fm);
static void flash_show_program_marker(flowmaster *fm);

static int flash_erase_chip(flowmaster *fm);
static int flash_program_data(flowmaster *fm, uint16_t address, uint8_t *data, int data_len);
static int flash_program_address(flowmaster *fm, uint16_t address);
static int flash_program_word(flowmaster *fm, uint8_t high, uint8_t low);

/* Tell flowmaster to enter programming mode */
static int flash_start_programming(flowmaster *fm);
/* Reboot the flowmaster microcontroller */
static void flash_end_programming(flowmaster *fm);

/* Intel Hex record types */
#define RECORD_TYPE_DATA 0x00
#define RECORD_TYPE_EOF 0x01

#define FLASH_PROGRAM_CHIP 10
#define FLASH_VALIDATE_ONLY 20


int
real_flash_validate_and_program(flowmaster *fm, FILE *fp, int do_program)
{
	uint8_t byte_count;
	uint16_t address;
	uint8_t record_type;
	uint8_t data[32]; /* The flash data in binary format */
	uint8_t file_checksum;
	uint8_t our_checksum;

	char hex_buffer[64]; /* This is dodgy, but it should never be this big */

	fseek(fp, 0, SEEK_SET);

	if(do_program == FLASH_PROGRAM_CHIP){
		flash_show_program_message(fm);
	}

	while(fgets(hex_buffer, sizeof(hex_buffer), fp) != NULL){
		if(hex_buffer[0] != ':'){
			/* Lines must start with : */
			return -1;
		}

		byte_count = flash_read_length(hex_buffer);
		address = flash_read_address(hex_buffer);
		record_type = flash_read_type(hex_buffer);

		switch(record_type){
			case RECORD_TYPE_DATA:
				break;
			case RECORD_TYPE_EOF:
				return 0;
			default:
				return -1;
		}

		flash_read_bytes(hex_buffer, data, sizeof(data));
		
		file_checksum = flash_read_checksum(hex_buffer);
		our_checksum = flash_calc_checksum(byte_count, address, record_type, data);

		if(our_checksum != file_checksum){
			return -1;
		}

		if(do_program == FLASH_PROGRAM_CHIP){
			flash_program_data(fm, address, data, byte_count);
		}
	}
	return -1;
}

/*
 * Preperation and cleanup routine, the real work is done int
 * real_flash_validate_and_program()
 *
 * */
int
flash_validate_and_program(flowmaster *fm, const char *filename)
{
	FILE *fp;
	int rc;
	fm_baud_rate oldrate;

	fp = fopen(filename, "r");
	if(fp == NULL){
		return -1;
	}
	/* Do a validation run */
	rc = real_flash_validate_and_program(fm, fp, FLASH_VALIDATE_ONLY);

	/* Abort if validation fails */
	if(rc != 0){
		fclose(fp);
		return -1;
	}

	/* Save the current baud rate, it will be changed by flash_start_programming */
	fm_get_baudrate(fm, &oldrate);

	/* Start programming mode. */
	rc = flash_start_programming(fm);
	if(rc != 0){
		fm_set_baudrate(fm, oldrate);
		fclose(fp);
		return -1;
	}

	/* TODO: Erase the chip! */
	rc = flash_erase_chip(fm);
	if(rc != 0) {
		/* Whups, BIG PROBLEM */
		fclose(fp);
		return -1;
	}

	rc = real_flash_validate_and_program(fm, fp, FLASH_PROGRAM_CHIP);
	if(rc != 0){
		/* Big error, oops.*/
	}

	/* Reset the flowmaster */
	flash_end_programming(fm);

	/* Restore the baud rate */
	fm_set_baudrate(fm, oldrate);

	fclose(fp);
	
	return rc;
}

/* Intel HEX file parsing routines */

static uint8_t
flash_convert_byte(uint8_t high, uint8_t low)
{
	uint8_t result = 0;

	result = flash_char_to_nibble(high) << 4;
	result |= flash_char_to_nibble(low);

	return result;
}

static uint16_t
flash_convert_word(uint8_t hh, uint8_t hl, uint8_t lh, uint8_t ll)
{
	uint16_t result;

	result = flash_convert_byte(hh, hl) << 8;
	result |= flash_convert_byte(lh, ll);

	return result;
}

static uint8_t
flash_read_length(const char *buffer)
{
	uint8_t len = flash_convert_byte(buffer[1],buffer[2]);
	return len;
}

static uint16_t
flash_read_address(const char *buffer)
{
	uint16_t addr = flash_convert_word(buffer[3], buffer[4], buffer[5], buffer[6]);
	return addr;
}

static uint8_t
flash_read_type(const char *buffer)
{
	uint8_t type = flash_convert_byte(buffer[7], buffer[8]);
	return type;
}

static uint8_t
flash_read_bytes(const char *src, uint8_t *dest, int destlen)
{
	int i;
	int length = flash_read_length(src) * 2;

	assert(destlen >= (length / 2));

	for(i = 0; i < length; i+=2){
		dest[i/2] = flash_convert_byte(src[i+9], src[i+10]);
	}
	return i / 2;
}

static uint8_t
flash_read_checksum(const char *buffer)
{
	uint8_t csum;
	int length = flash_read_length(buffer);
	
	length = (length * 2) + 9;
	csum = flash_convert_byte(buffer[length], buffer[length+1]);
	
	return csum;
}

static uint8_t
flash_calc_checksum(uint8_t size, uint16_t address, uint8_t type, uint8_t *data)
{
	uint8_t checksum;
	uint8_t i;

	checksum = size;
	checksum += ((uint8_t)(address >> 8)) + ((uint8_t)(address & 0x00FF));
	checksum += type;

	for(i = 0; i < size; i++){
		checksum += data[i];
	}

	checksum = (~checksum) + 1;

	return checksum;
}

static uint8_t
flash_char_to_nibble(uint8_t byte)
{
	switch(byte){
		case '0':
			return 0x00;
		case '1':
			return 0x01;
		case '2':
			return 0x02;
		case '3':
			return 0x03;
		case '4':
			return 0x04;
		case '5':
			return 0x05;
		case '6':
			return 0x06;
		case '7':
			return 0x07;
		case '8':
			return 0x08;
		case '9':
			return 0x09;
		case 'A':
			return 0x0A;
		case 'B':
			return 0x0B;
		case 'C':
			return 0x0C;
		case 'D':
			return 0x0D;
		case 'E':
			return 0x0E;
		case 'F':
			return 0x0F;
	}
	assert(0);
	return 0;
}

/* 
 * Flash programming routines.
 * 
 * These routines send the data to the microcontroller
 * */

static int
flash_start_programming(flowmaster *fm)
{
	unsigned char byte = 0;

	fm_flush_buffers(fm);

	fm_start_write_buffer(fm, PACKET_TYPE_BOOTLOADER, 0);
	fm_end_write_buffer(fm);
	fm_serial_write(fm);

	/* The bootloader is hardcoded to 19200 */
	fm_set_baudrate(fm, FM_B19200);

	do {
		fm_serial_write_byte(fm, BL_PING);
		fm_serial_read_byte(fm, &byte);
	} while(byte != BL_ACK);

	return 0;
}

static int
flash_erase_chip(flowmaster *fm)
{
	unsigned char byte = 0;
	int rc;

	fm_flush_buffers(fm);
	fm_serial_write_byte(fm, BL_ERASE);

	do {
		rc = fm_serial_read_byte(fm, &byte);
	}while(rc == 0);

	if(byte != BL_ACK){
		fprintf(stderr, "Failed to erase chip, got '%02X' expected '%02X'\n",
			(int)byte, (int)BL_ACK);
		return -1;
	}

	return 0;
}

static int
flash_program_data(flowmaster *fm, uint16_t address, uint8_t *data, int data_len)
{
	int i;
	int rc;
	
	fm_flush_buffers(fm);

	if((rc = flash_program_address(fm, address)) != 0){
		return -1;
	}

	for(i = 0; i < data_len; i += 2){
		if((rc = flash_program_word(fm, data[i], data[i+1])) != 0){
			return -1;
		}
	}

	return 0;
}

static int
flash_program_address(flowmaster *fm, uint16_t address)
{
	int rc;
	unsigned char response;

	fm_serial_write_byte(fm, BL_SET_ADDR);
	fm_serial_write_byte(fm, (unsigned char) ((address >> 8) & 0x00FF));
	fm_serial_write_byte(fm, (unsigned char) (address & 0x00FF));

	do {
		rc = fm_serial_read_byte(fm, &response);
	} while(rc == 0);

	if(response != BL_ACK){
		return -1;
	}

	return 0;
}

static int
flash_program_word(flowmaster *fm, uint8_t high, uint8_t low)
{
	int rc;
	unsigned char response;

	fm_serial_write_byte(fm, BL_PROGRAM);
	/* TODO: this is a bit screwey, i'm getting byte orders fucked up somewhere */
	fm_serial_write_byte(fm, low);
	fm_serial_write_byte(fm, high);
	
	do {
		rc = fm_serial_read_byte(fm, &response);
	} while(rc == 0);

	if(response != BL_ACK){
		return -1;
	}

	return 0;
}

static void
flash_show_program_message(flowmaster *fm)
{
	fm_serial_write_byte(fm, BL_PROGRAM_MESSAGE);
}

static void
flash_show_program_marker(flowmaster *fm)
{
	fm_serial_write_byte(fm, BL_PERCENT);
}

static void
flash_end_programming(flowmaster *fm)
{
	fm_serial_write_byte(fm, BL_RESET);
}

