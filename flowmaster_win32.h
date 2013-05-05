#ifndef FLOWMASTER_WIN32_H
#define FLOWMASTER_WIN32_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

enum fm_baud_rate_e {
	FM_B19200 = CBR_19200,
	FM_B38400 = CBR_38400,
	FM_B57600 = CBR_57600,
	FM_B115200 = CBR_115200
};

#endif