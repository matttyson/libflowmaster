#ifndef FM_SERIAL_H
#define FM_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flowmaster_internal.h"


typedef struct flowmaster_s flowmaster;

/*
	FM_B19200  - 19200  bps
	FM_B38400  - 38400  bps
	FM_B57600  - 57600  bps
	FM_B115200 - 115200 bps

	Valid baud rate values.
	Defined in platform include file
*/
typedef enum fm_baud_rate_e fm_baud_rate;



/*
 *	Data result to return the fan status
 * */
struct fm_data_s {
	/* Duty cycle, a percentage between 0.0 and 1.0 */
	float fan_duty_cycle;
	float pump_duty_cycle;

	/* Temp, in degrees celcius */
	float ambient_temp;
	float coolant_temp;

	/* litres per hour */
	float flow_rate;

	/* Fan RPM, as a whole integer*/
	int fan_rpm;
	int pump_rpm;
};
typedef struct fm_data_s fm_data;



/*
 * Flowmaster return codes
 *
 * Generally, zero on success. otherwise check RC.
 * 
 * */
enum fm_rc_e 
{
	FM_OK = 0,
	FM_WRITE_ERROR,
	FM_READ_TIMEOUT,
	FM_READ_ERROR,
	FM_PORT_ERROR,
	FM_CHECKSUM_ERROR,
	FM_FILE_ERROR,
	FM_BAD_HEXFILE
};
typedef enum fm_rc_e fm_rc;



/* Create a flowmaster handle */
DLLEXPORT flowmaster* fm_create();

/* clean up the flowmaster handle */
DLLEXPORT void fm_destroy(flowmaster *fm);

/* 
 * Open the serial port 
 * port - the serial port to open.  EG "COM1" "/dev/ttyUSB0" etc
 * */

DLLEXPORT fm_rc fm_connect(flowmaster *fm, const char *port);
/* Close the serial port */
DLLEXPORT fm_rc fm_disconnect(flowmaster *fm);

/* True if connected*/
DLLEXPORT int fm_isconnected(flowmaster *fm);

/* Get the current pump data */
DLLEXPORT fm_rc fm_get_data(flowmaster *fm, fm_data *data);

/* returns 0 if alive, -1 if error*/
DLLEXPORT fm_rc fm_ping(flowmaster *fm);

/* Set the cursor XY position */
DLLEXPORT int fm_set_cursor(flowmaster *fm, int row, int col);

/* Print a message to the flowmaster display. max 20 chars. */
DLLEXPORT int fm_print_message(flowmaster *fm, const char *message, int message_len);

/* 
 * Set the speed of the fans or pump.
 * duty_cycle is a float between 0.0 and 1.0
 *
 * duty cycles > 1.0 will be set to 1.0
 * duty cycles < 0.3 will be set to 0.3
 *
 * A fan or pump may never be switched off.
 * */
DLLEXPORT int fm_set_fan_speed(flowmaster *fm, float duty_cycle);
DLLEXPORT int fm_set_pump_speed(flowmaster *fm, float duty_cycle);

/* Stop rotating the displays */
DLLEXPORT int fm_halt_update_display(flowmaster *fm);

/* Change to a given display */
DLLEXPORT int fm_set_display(flowmaster *fm, int display);

/* filename is a path to an intel HEX file that you want to upload to the microcontroller */
DLLEXPORT int flash_validate_and_program(flowmaster *fm, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
