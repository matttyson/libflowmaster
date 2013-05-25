#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "protocol.h"
#include "flowmaster_private.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define PACKET_TYPE 0
#define PACKET_DATA_LEN 1
#define PACKET_DATA 2

static int fm_set_speed(flowmaster *fm, float duty_cycle, int fan_or_pump);
#ifdef FM_DEBUG_LOGGING
static void fm_dump_buffer(const unsigned char *buffer, int length);
#endif

static unsigned char fm_calc_crc8(const unsigned char* data_pointer, int number_of_bytes);
/* Convert the ADC value into celcius */
static float convert_temp_c(int adcval);

void
dump_rx_packet(flowmaster *fm)
{
	int i;
	for(i = 0; i < fm->read_buffer_len; i++){
		fprintf(stdout, "0x%02X ", fm->read_buffer[i]);
	}
	fprintf(stdout, "\n");
}

void
dump_tx_packet(flowmaster *fm)
{
	int i;
	for(i = 0; i < fm->write_buffer_len; i++){
		fprintf(stdout, "0x%02X ", fm->write_buffer[i]);
	}
	fprintf(stdout, "\n");
}

flowmaster*
fm_create()
{
	flowmaster *fm = (flowmaster*) calloc(1, sizeof(flowmaster));

#ifdef _WIN32
	fm->port = INVALID_HANDLE_VALUE;
#endif

	return fm;
}

void
fm_destroy(flowmaster *fm)
{
	if(fm_isconnected(fm)){
		fm_disconnect(fm);
	}
	free(fm);
}

fm_rc
fm_get_data(flowmaster *fm, fm_data *data)
{
	int rc;
	int temp;
	int written;

	fm_start_write_buffer(fm, PACKET_TYPE_REQUEST_STATUS, 0);
	fm_end_write_buffer(fm);

	fm_flush_buffers(fm);

	rc = fm_serial_write(fm, &written);
	if(rc != 0){
		return FM_WRITE_ERROR;
	}
	
	rc = fm_serial_read(fm);
	if(rc != 0){
		fprintf(stderr,"serial_read: %d\n",rc);
		return FM_READ_ERROR;
	}

	if(fm->read_buffer_len == 0){
		fprintf(stderr,"Zero byte response!\n");
		return FM_READ_ERROR;
	}

	/*
	 *	Type should be heartbeat, length should be ??
	 * */	
	if(fm_validate_packet(fm, PACKET_TYPE_HEARTBEAT) != 0){
		return FM_CHECKSUM_ERROR;
	}

	/* Convert the duty cycle back into a percentage */
	temp = ((fm->read_buffer[2] << 8) | fm->read_buffer[3]);
	data->fan_duty_cycle  = (float)temp / (float)FM_TOP;

	temp = (fm->read_buffer[4] << 8) | fm->read_buffer[5];
	data->pump_duty_cycle = (float)temp / (float)FM_TOP;

	data->fan_rpm  = fm->read_buffer[6] * 30;
	data->pump_rpm = fm->read_buffer[7] * 30;

	/* Take raw ADC values and convert into celcius */
	temp = (fm->read_buffer[8] << 8)  | (fm->read_buffer[9]);
	data->ambient_temp = convert_temp_c(temp);

	temp = (fm->read_buffer[10] << 8) | (fm->read_buffer[11]);
	data->coolant_temp = convert_temp_c(temp);

	/* Flow rate isn't used yet*/
	//data->flow_rate = fm->read_buffer[12];
	data->flow_rate = 0.0;
	return FM_OK;
}


fm_rc
fm_ping(flowmaster *fm)
{
	int rc;
	int written;

	fm_start_write_buffer(fm, PACKET_TYPE_PING,0);
	fm_end_write_buffer(fm);

	fm_flush_buffers(fm);

	if((rc = fm_serial_write(fm, &written)) != 0){
		return FM_WRITE_ERROR;
	}

	if((rc = fm_serial_read(fm)) != 0){
		return FM_READ_ERROR;
	}

	if(fm_validate_packet(fm, PACKET_TYPE_PONG) != 0){
		return FM_CHECKSUM_ERROR;
	}

	return FM_OK;
}

void
fm_start_write_buffer(flowmaster *fm, int packet_type, int data_len)
{
	fm->write_buffer_len = 2;
	fm->write_buffer[0] = DLE;
	fm->write_buffer[1] = STX;
	fm_add_byte(fm,packet_type);
	fm_add_byte(fm,data_len);
}

void
fm_end_write_buffer(flowmaster *fm)
{
	fm_add_csum(fm,fm->write_buffer[3] + 2 );
	fm->write_buffer[fm->write_buffer_len++] = DLE;
	fm->write_buffer[fm->write_buffer_len++] = ETX;
}

void
fm_add_byte(flowmaster *fm, unsigned char byte)
{
	if(byte == DLE){
		fm->write_buffer[fm->write_buffer_len++] = DLE;
	}
	fm->write_buffer[fm->write_buffer_len++] = byte;
}

void
fm_add_word(flowmaster *fm, uint16_t word)
{
	/* Put the word into network byte order */
	fm_add_byte(fm, (unsigned char)(word >> 8));
	fm_add_byte(fm, (unsigned char)(word & 0x00FF));
}

void
fm_add_csum(flowmaster *fm, int length)
{
	const unsigned char csum = fm_calc_crc8(&(fm->write_buffer[2]),length);
	fm_add_byte(fm, csum);
}

/*
 *	Dallas CRC-8.
 *	Stolen from here: http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=34907
 *
 * */
unsigned char
fm_calc_crc8(const unsigned char* data_pointer, int number_of_bytes)
{
	unsigned char temp1, bit_counter, feedback_bit, crc8_result = 0;

	while (number_of_bytes--) {
		temp1 = *data_pointer++;

		for (bit_counter = 8; bit_counter; bit_counter--) {
			feedback_bit = (crc8_result & 0x01);
			crc8_result >>= 1;
			if (feedback_bit ^ (temp1 & 0x01)) {
				crc8_result ^= 0x8c;
			}
			temp1 >>= 1;
		}
	}
	return crc8_result;
}


int
fm_serial_read(flowmaster *fm)
{
	unsigned char byte;
	unsigned char prev_byte = 0;
	int rc;
	int i;

#ifdef FM_DEBUG_LOGGING
	fprintf(stdout,"%s\n",__func__);
#endif

	fm->read_buffer_len = 0;
	
	while(1){
		/* keep retrying the read while zero bytes read */
		for(i = 0; ((rc = fm_serial_read_byte(fm, &byte)) != 0) ; i++){
			if(i == 3){
				/* Sanity timeout */
				return -1;
			}
		}

#ifdef FM_DEBUG_LOGGING
		fprintf(stdout,"0x%2X (%d)\n",(int)byte,(int)byte);
#endif
		
		if(rc != 0){
			/* Bad read? */
			fprintf(stderr,"Read returned %d\n",rc);
			return -1;
		}
		
		if(prev_byte == DLE){
			prev_byte = byte;
			switch(byte){
				case STX:
					/* Start of header, begin reception */
					fm->read_buffer_len = 0;
					continue;
				case ETX:
					/* Transmission complete */
					goto done;
				case DLE:
					goto dle_unstuff;
			}
		}

		prev_byte = byte;
		if(byte != DLE){
			dle_unstuff:
			fm->read_buffer[fm->read_buffer_len++] = byte;

			if(fm->read_buffer_len == FM_BUFFER_SIZE){
				/* Buffer overflow */
				return -1;
			}
		}
	}

	done:
	return 0;
}

int
fm_validate_packet(flowmaster *fm, int expected_packet)
{
	/* Check that the packet type and data match the checksum */
	unsigned char csum;
	unsigned char recv_csum;

	csum = fm_calc_crc8(fm->read_buffer, fm->read_buffer_len - 1);
	recv_csum = fm->read_buffer[fm->read_buffer[1] + 2];

	if(csum != recv_csum){
		fprintf(stdout,"CSUM MISMATCH: 0x%02X != 0x%02X\n",(int)csum,(int)recv_csum);
#ifdef FM_DEBUG_LOGGING
		fm_dump_buffer(fm->read_buffer, fm->read_buffer_len);
#endif
		return -1;
	}

	if(fm->read_buffer[0] != expected_packet){
		fprintf(stdout,"Packet type mismatch\n");
		/*dump_rx_packet(fm);*/
		return -2;
	}
	
	return 0;
}

int
fm_prev_rx_byte(flowmaster *fm, unsigned char *byte)
{
	if(fm->read_buffer_len > 0){
		*byte = fm->read_buffer[fm->read_buffer_len - 1];
	}
	else {
		*byte = 0;
	}
	return 0;
}

int
fm_set_speed(flowmaster *fm, float duty_cycle, int fan_or_pump)
{
	uint16_t cycle;
	int rc;
	int written;


	if(duty_cycle > 1.0){
		duty_cycle = 1.0;
	}
	else if(duty_cycle < 0.3){
	//	duty_cycle = 0.2;
	}

	cycle = (uint16_t) (FM_TOP * duty_cycle);

	fm_start_write_buffer(fm, fan_or_pump, 2);
	fm_add_word(fm, cycle);
	fm_end_write_buffer(fm);

	fm_flush_buffers(fm);

	if((rc = fm_serial_write(fm, &written)) == 0){
		/* Write error */
		return FM_WRITE_ERROR;
	}

	if((rc = fm_serial_read(fm)) != 0){
		/* read error */
		return FM_READ_ERROR;
	}

	if((rc = fm_validate_packet(fm, PACKET_TYPE_ACK)) != 0){
		return FM_CHECKSUM_ERROR;
	}
	
	return FM_OK;
}

#ifdef FM_DEBUG_LOGGING
static void
fm_dump_buffer(const unsigned char *buffer, int length)
{
	int i;
	for(i = 0; i < length; i++){
		fprintf(stdout,"%02X ",(int)buffer[i] & 0xFF);
	}
	fprintf(stdout,"\n");
}
#endif

float
convert_temp_c(int adc_val)
{
	const int ref_resistor = 10000;
	double temp;

	/* This is a dirty hack to prevent a divide by zero */
	if(adc_val == 0){
		adc_val = 1;
	}

	temp = log((double)((1024 * ref_resistor / adc_val) - ref_resistor));
	temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
	temp -= 273.15;

	return (float) temp;
}

/*
 * Getter and setter routines for libflowmaster
 *
 */

int
fm_set_fan_speed(flowmaster *fm, float duty_cycle)
{
	return fm_set_speed(fm, duty_cycle, PACKET_TYPE_SET_FAN);
}

int
fm_set_pump_speed(flowmaster *fm, float duty_cycle)
{
	return fm_set_speed(fm, duty_cycle, PACKET_TYPE_SET_PUMP);
}

fm_rc
fm_update_status(flowmaster *fm)
{
	return fm_get_data(fm, &(fm->data));
}

float
fm_fan_duty_cycle(flowmaster *fm)
{
	return fm->data.fan_duty_cycle;
}

float
fm_pump_duty_cycle(flowmaster *fm)
{
	return fm->data.pump_duty_cycle;
}

float
fm_ambient_temp(flowmaster *fm)
{
	return fm->data.ambient_temp;
}

float
fm_coolant_temp(flowmaster *fm)
{
	return fm->data.coolant_temp;
}

int
fm_fan_rpm(flowmaster *fm)
{
	return fm->data.fan_rpm;
}

int
fm_pump_rpm(flowmaster *fm)
{
	return fm->data.pump_rpm;
}
