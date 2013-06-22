#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "protocol.h"
#include "flowmaster_private.h"

#ifdef _WIN32
#include <Windows.h>
static __inline float roundf(float num) { return floorf(num + 0.5f ); }
#endif

#define PACKET_TYPE 0
#define PACKET_DATA_LEN 1
#define PACKET_DATA 2

static int fm_set_speed(flowmaster *fm, float duty_cycle, int fan_or_pump);
#ifdef FM_DEBUG_LOGGING
static void fm_dump_buffer(const unsigned char *buffer, int length, uint8_t csum, uint8_t recv_csum);
#endif

static unsigned char fm_calc_crc8(const unsigned char* data_pointer, int number_of_bytes);
/* Convert the ADC value into celcius */
static float convert_temp_c(int adcval);

static fm_rc fm_get_top(flowmaster *fm);

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
fm_connect(flowmaster *fm, const char *port)
{
	fm_rc rc;

	rc = fm_connect_private(fm, port);
	if(rc != FM_OK){
	    return rc;
	}

	rc = fm_ping(fm);
	if(rc != FM_OK) {
	    return rc;
	}

	rc = fm_get_top(fm);
	if(rc != FM_OK){
		return rc;
	}

	return FM_OK;
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
		return FM_READ_ERROR;
	}

	if(fm->read_buffer_len == 0){
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
	data->fan_duty_cycle  = (float)temp / (float)fm->timer_top;

	temp = (fm->read_buffer[4] << 8) | fm->read_buffer[5];
	data->pump_duty_cycle = (float)temp / (float)fm->timer_top;

	data->fan_rpm  = fm->read_buffer[6] * 30;
	data->pump_rpm = fm->read_buffer[7] * 30;

	/* Take raw ADC values and convert into celcius */
	temp = (fm->read_buffer[8] << 8)  | (fm->read_buffer[9]);
	data->coolant_temp = convert_temp_c(temp);

	temp = (fm->read_buffer[10] << 8) | (fm->read_buffer[11]);
	data->ambient_temp = convert_temp_c(temp);

	/* Flow rate isn't used yet*/
	//data->flow_rate = fm->read_buffer[12];
	data->flow_rate = 0.0f;
	return FM_OK;
}

static fm_rc
fm_get_top(flowmaster *fm)
{
	int written;
	int rc;

	fm_start_write_buffer(fm, PACKET_TYPE_GET_TOP,0);
	fm_end_write_buffer(fm);
	fm_flush_buffers(fm);

	rc = fm_serial_write(fm, &written);
	if(rc != 0) {
		return FM_WRITE_ERROR;
	}

	rc = fm_serial_read(fm);
	if(rc != 0){
		return FM_READ_ERROR;
	}

	if(fm->read_buffer_len == 0){
		return FM_READ_ERROR;
	}

	if(fm_validate_packet(fm, PACKET_TYPE_GET_TOP) != 0){
		return FM_CHECKSUM_ERROR;
	}

	fm->timer_top = ((fm->read_buffer[2] << 8) | fm->read_buffer[3]);

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

	fm->read_buffer_len = 0;
	
	while(1){
		/* keep retrying the read while zero bytes read */
		for(i = 0; ((rc = fm_serial_read_byte(fm, &byte)) != 0) ; i++){
			if(i == 3){
				/* Sanity timeout */
				return -1;
			}
		}

		if(rc != 0){
			/* Bad read? */
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

	return 0;

	csum = fm_calc_crc8(fm->read_buffer, fm->read_buffer_len - 1);
	recv_csum = fm->read_buffer[fm->read_buffer[1] + 2];

	if(csum != recv_csum){
#ifdef FM_DEBUG_LOGGING
		fprintf(stdout,"CSUM MISMATCH: 0x%02X != 0x%02X\n",(int)csum,(int)recv_csum);
		fm_dump_buffer(fm->read_buffer, fm->read_buffer_len, csum, recv_csum);
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

static int
fm_do_write(flowmaster *fm, int response)
{
	int written;
	int rc;

	if((rc = fm_serial_write(fm, &written)) != 0){
		/* Write error */
		return FM_WRITE_ERROR;
	}

	if((rc = fm_serial_read(fm)) != 0){
		/* read error */
		return FM_READ_ERROR;
	}

	if((rc = fm_validate_packet(fm, response)) != 0){
		return FM_CHECKSUM_ERROR;
	}

	return FM_OK;
}

int
fm_autoregulate(flowmaster *fm, int regulate)
{
	const int type = regulate ? PACKET_TYPE_AUTOMATIC : PACKET_TYPE_MANUAL;
	fm_start_write_buffer(fm, type, 0);
	fm_end_write_buffer(fm);
	return fm_do_write(fm, PACKET_TYPE_ACK);
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

	cycle = (uint16_t) (fm->timer_top * duty_cycle);

	fm_start_write_buffer(fm, fan_or_pump, 2);
	fm_add_word(fm, cycle);
	fm_end_write_buffer(fm);

	if((rc = fm_serial_write(fm, &written)) != 0){
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

static fm_rc
fm_set_fan_profile_segment(flowmaster *fm, float *data, int offset, int count);

fm_rc
fm_set_fan_profile(struct flowmaster_s *fm, float *data, int length)
{
	int offset = 0;
	int count = 5;
	fm_rc rc;

	if(length != FM_FAN_BUFFER_SIZE){
		return FM_BAD_BUFFER_LENGTH;
	}

	while(offset < FM_FAN_BUFFER_SIZE) {

		if((offset + count) > FM_FAN_BUFFER_SIZE){
			count = FM_FAN_BUFFER_SIZE - offset;
		}

		rc = fm_set_fan_profile_segment(fm, data, offset, count);

		if(rc != FM_OK){
			return rc;
		}

		offset += count;
	}

	return FM_OK;
}

fm_rc
fm_set_fan_profile_segment(flowmaster *fm, float *data, int offset, int count)
{
	int i;
	fm_rc rc;
	const int bytes_to_send = (count * 2) + 2;
	float *ptr = data + offset;
	int written;

	fm_start_write_buffer(fm, PACKET_TYPE_SET_FAN_PROFILE, bytes_to_send);
	fm_add_byte(fm, (uint8_t)(count  & 0xFF));
	fm_add_byte(fm, (uint8_t)(offset & 0xFF));

	for(i = 0; i < count; i++){
		const int temp = (int) roundf((*ptr * fm->timer_top));
		fm_add_word(fm, (uint16_t)(temp & 0xFFFF));
		ptr++;
	}
	fm_end_write_buffer(fm);

	if((rc = fm_serial_write(fm, &written)) != 0){
		return FM_WRITE_ERROR;
	}

	if((rc = fm_serial_read(fm)) != 0){
		return FM_READ_ERROR;
	}

	if((rc = fm_validate_packet(fm, PACKET_TYPE_ACK)) != 0){
		return FM_CHECKSUM_ERROR;
	}

	return FM_OK;
}


/**
 * data - the fan speed array, must be 65 elements.
 * length - the length of the buf, must be 65
 *
 *
 */

static fm_rc
fm_get_fan_profile_segment(flowmaster *fm, int offset, int *read, float *data);

fm_rc
fm_get_fan_profile(flowmaster *fm, float *data, int length)
{
	int offset = 0;
	int ctr = 0;

	if(length != FM_FAN_BUFFER_SIZE) {
		return FM_BAD_BUFFER_LENGTH;
	}

	do {
		int received;
		fm_get_fan_profile_segment(fm, offset, &received, data);
		offset += received;
		ctr++;

	} while(offset < FM_FAN_BUFFER_SIZE);

	return 0;
}

fm_rc
fm_get_fan_profile_segment(flowmaster *fm, int offset, int *read, float *data)
{
	int received = 0;
	int count;
	int i;
	int written;
	int ptr = 3;
	fm_rc rc;
	/* bytes 0 and 1 are packet type and length */

	fm_start_write_buffer(fm, PACKET_TYPE_GET_FAN_PROFILE, 1);
	fm_add_byte(fm, offset);
	fm_end_write_buffer(fm);

	rc = fm_serial_write(fm, &written);
	if(rc != 0){
		return FM_WRITE_ERROR;
	}

	rc = fm_serial_read(fm);
	if(rc != 0){
		return FM_READ_ERROR;
	}

	if((rc = fm_validate_packet(fm, PACKET_TYPE_GET_FAN_PROFILE)) != 0){
		return FM_CHECKSUM_ERROR;
	}

	/* Number of items in the packet */
	count = fm->read_buffer[2];

	data += offset;

	for(i = 0; i < count ; i++){
		int temp = (fm->read_buffer[ptr++] << 8);
		temp |= fm->read_buffer[ptr++];
		(*data) = ((float)temp / (float)fm->timer_top) ;
		data++;
		received++;
	}

	*read = received;

	return FM_OK;
}


#ifdef FM_DEBUG_LOGGING
static void
fm_dump_buffer(const unsigned char *buffer, int length, uint8_t csum, uint8_t recv_csum)
{
	int i;
	FILE *fp;

	fp = fopen("error.log","a+");
	if(fp == NULL){
		return;
	}

	fprintf(fp,"Checksum Error. Expected %02X got %02x\n",(int)csum, (int)recv_csum);

	for(i = 0; i < length; i++){
		fprintf(fp,"%02X ",(int)buffer[i] & 0xFF);
	}
	fprintf(fp,"\n");

	fclose(fp);
}
#endif

float
convert_temp_c(int adc_val)
{
	double temp;
	const int ref_resistor = 10000;

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
