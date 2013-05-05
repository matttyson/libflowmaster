#ifndef FM_SERIAL_PRIVATE
#define FM_SERIAL_PRIVATE

#include "flowmaster.h"
#include <stdint.h>

#if defined _WIN32
	#include <Windows.h>
	typedef HANDLE serial_handle;
#elif defined __unix
	typedef int serial_handle;
#else
	#error unsupported platform
#endif

#define FM_BUFFER_SIZE 32

#define FM_TOP 288


struct flowmaster_s
{
	serial_handle port;
	unsigned char write_buffer[FM_BUFFER_SIZE];
	unsigned char read_buffer[FM_BUFFER_SIZE];
	int write_buffer_len; /* number of chars in the buffer */
	int read_buffer_len; /* number of chars in the buffer */
};
typedef struct flowmaster_s flowmaster;

int fm_prev_rx_byte(flowmaster *fm, unsigned char *byte);

/* Writes a block of bytes */
int fm_serial_write(flowmaster *fm);
/* Writes a single byte */
int fm_serial_write_byte(flowmaster *fm, unsigned char byte);

/* Reads a single byte */
int fm_serial_read_byte(flowmaster *fm, unsigned char *byte);

/* Reads from the input buffer and discards results */
void fm_flush_input(flowmaster *fm);
void fm_flush_buffers(flowmaster *fm);

/* Get and Set the baud rate*/

int fm_set_baudrate(flowmaster *fm, fm_baud_rate baud);
int fm_get_baudrate(flowmaster *fm, fm_baud_rate *baud);

/* For reading and writing data to the microcontroller */
void fm_start_write_buffer(flowmaster *fm, int packet_type, int data_len);
void fm_end_write_buffer(flowmaster *fm);
void fm_add_byte(flowmaster *fm, unsigned char byte);
void fm_add_word(flowmaster *fm, uint16_t byte);
void fm_add_csum(flowmaster *fm, int length);
int  fm_serial_read(flowmaster *fm);
int  fm_validate_packet(flowmaster *fm, int expected_packet);

#endif
