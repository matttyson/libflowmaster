#ifndef FM_SERIAL_H
#define FM_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flowmaster_internal.h"


struct flowmaster_s;

#ifndef __cplusplus
	/*
	 * The intention here is to make the 'flowmaster' typedef available
	 * for use in C code, but not mess up the namespace for the C++ code
	 *
	 * If we are using C++ code, then don't expose this so we can declare
	 * the flowmaster class.
	 * */
	typedef struct flowmaster_s flowmaster;
#endif

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
	FM_BAD_HEXFILE,
	FM_BAD_BUFFER_LENGTH
};
typedef enum fm_rc_e fm_rc;

/* Create a flowmaster handle */
DLLEXPORT struct flowmaster_s* fm_create();

/* clean up the flowmaster handle */
DLLEXPORT void fm_destroy(struct flowmaster_s *fm);

/* 
 * Open the serial port 
 * port - the serial port to open.  EG "COM1" "/dev/ttyUSB0" etc
 * */

DLLEXPORT fm_rc fm_connect(struct flowmaster_s *fm, const char *port);
/* Close the serial port */
DLLEXPORT fm_rc fm_disconnect(struct flowmaster_s *fm);

/* True if connected*/
DLLEXPORT int fm_isconnected(struct flowmaster_s *fm);

/* returns 0 if alive, -1 if error*/
DLLEXPORT fm_rc fm_ping(struct flowmaster_s *fm);

/* Set the cursor XY position */
DLLEXPORT int fm_set_cursor(struct flowmaster_s *fm, int row, int col);

/* Print a message to the flowmaster display. max 20 chars. */
DLLEXPORT int fm_print_message(struct flowmaster_s *fm, const char *message, int message_len);

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
DLLEXPORT int fm_halt_update_display(struct flowmaster_s *fm);

/* Change to a given display */
DLLEXPORT int fm_set_display(struct flowmaster_s *fm, int display);

/* filename is a path to an intel HEX file that you want to upload to the microcontroller */
DLLEXPORT int flash_validate_and_program(struct flowmaster_s *fm, const char *filename);

/*
 * Query the microcontroller for it's latest status info.
 * Don't call this more than once per 500ms
 *
 * */
DLLEXPORT fm_rc fm_update_status(struct flowmaster_s *fm);

DLLEXPORT fm_rc fm_set_fan_profile(struct flowmaster_s *fm, int *data, int length);
DLLEXPORT fm_rc fm_get_fan_profile(struct flowmaster_s *fm, int *data, int length);


/*
 * Getter functions for fetching the status of the pump controller.
 * Refreshed by calling fm_update_status();
 * */

DLLEXPORT float fm_fan_duty_cycle(flowmaster *fm);
DLLEXPORT float fm_pump_duty_cycle(flowmaster *fm);
DLLEXPORT float fm_ambient_temp(flowmaster *fm);
DLLEXPORT float fm_coolant_temp(flowmaster *fm);

DLLEXPORT int fm_fan_rpm(flowmaster *fm);
DLLEXPORT int fm_pump_rpm(flowmaster *fm);

#ifdef __cplusplus
}
#endif

#endif
