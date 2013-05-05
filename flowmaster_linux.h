#ifndef FLOWMASTER_LINUX_H
#define FLOWMASTER_LINUX_H

#include <termios.h>

/*
 *	Unix baud rate values
 * */

enum fm_baud_rate_e {
	FM_B19200 = B19200,
	FM_B38400 = B38400,
	FM_B57600 = B57600,
	FM_B115200 = B115200
};

#endif
