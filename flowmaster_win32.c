#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <Windows.h>

#include "flowmaster.h"
#include "flowmaster_private.h"
#include "protocol.h"


fm_rc
fm_connect(flowmaster *fm, const char *port)
{
	HANDLE handle;
	COMMTIMEOUTS timeouts;
	DCB dcb = {0};

	handle = CreateFileA( port, GENERIC_READ | GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,0,0);

	if(handle == INVALID_HANDLE_VALUE){
		return FM_PORT_ERROR;
	}

	/* Configure the com port */
	if(!GetCommState(handle, &dcb)){
		CloseHandle(handle);
		return FM_PORT_ERROR;
	}

	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fNull = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError = FALSE;

	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.BaudRate = CBR_19200;

	if(!SetCommState(handle, &dcb)){
		CloseHandle(handle);
		return FM_PORT_ERROR;
	}

	/* apply a 200 milisecond read timeout */
	timeouts.ReadIntervalTimeout = 200;
	
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;

	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	SetCommTimeouts(handle, &timeouts);

	fm->port = handle;
	return FM_OK;
}

int
fm_set_baudrate(flowmaster *fm, fm_baud_rate rate)
{
	DCB dcb;

	if(!GetCommState(fm->port, &dcb)){
		return FM_PORT_ERROR;
	}

	dcb.BaudRate = rate;

	if(!SetCommState(fm->port, &dcb)){
		return FM_PORT_ERROR;
	}

	return FM_OK;
}

int
fm_get_baudrate(flowmaster *fm, fm_baud_rate *baud)
{
	DCB dcb;

	if(!GetCommState(fm->port, &dcb)){
		return FM_PORT_ERROR;
	}

	*baud = (fm_baud_rate) dcb.BaudRate;

	return FM_OK;
}

fm_rc
fm_disconnect(flowmaster *fm)
{
	CloseHandle(fm->port);
	fm->port = INVALID_HANDLE_VALUE;
	return FM_OK;
}

int
fm_isconnected(flowmaster *fm)
{
	return fm->port != INVALID_HANDLE_VALUE;
}

int
fm_serial_write(flowmaster *fm, int *bytes_written)
{
	DWORD written;
	BOOL rc;

	rc = WriteFile(fm->port, fm->write_buffer, fm->write_buffer_len, &written, NULL);
	if(!rc){
		return -1;
	}

	*bytes_written = (int) written;

	return 0;
}

int
fm_serial_read_byte(flowmaster *fm, unsigned char *byte)
{
	DWORD read;
	BOOL rc;

	rc = ReadFile(fm->port, byte, 1, &read, NULL);
	if(!rc){
		return -1;
	}

	return (int) read;
}

int
fm_serial_write_byte(flowmaster *fm, unsigned char byte)
{
	DWORD written;
	BOOL rc = WriteFile(fm->port,&byte,1,&written,NULL);

	if(!rc){
		return -1;
	}

	return (int) written;
}

void
fm_flush_buffers(flowmaster *fm)
{
	PurgeComm(fm->port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR );
}


/*
	Win32 DLL entry point function
*/
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


