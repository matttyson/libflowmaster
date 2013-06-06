#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "flowmaster_private.h"
#include "protocol.h"


fm_rc
fm_connect(flowmaster *fm, const char *port)
{
	serial_handle handle;
	struct termios tio;
	struct flock fl;

	handle = open(port, O_RDWR | O_NOCTTY);

	if(handle == -1){
		return FM_PORT_ERROR;
	}

	/* Lock the serial port, can be a bit pointless as linux locks are advisory only */
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_whence = SEEK_SET;

	fcntl(handle, F_GETLK, &fl);
	if(fl.l_type != F_UNLCK){
		/* File is already locked */
		fprintf(stdout,"file is already locked\n");
		close(handle);
		return FM_PORT_ERROR;
	}

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = getpid();

	if(fcntl(handle, F_SETLKW, &fl) != 0){
		fprintf(stdout,"File lock failed\n");
		close(handle);
		return FM_PORT_ERROR;
	}

	tcgetattr(handle, &tio);

	//fcntl(handle, F_SETFL, FNDELAY);

	tcgetattr(handle, &tio);

	tio.c_cflag |= CLOCAL | CREAD;

	/* 100ms timeout */
	//tio.c_cc[VTIME] = 100;

	/* 8N1 */
	tio.c_cflag &= ~PARENB;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;

	/* Flow control off*/
  	tio.c_iflag &= ~(IXON | IXOFF | IXANY);
  
	/* Raw input, non cannonical */
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* Raw output */
	tio.c_oflag &= ~OPOST;
	
	tcsetattr(handle, TCSANOW, &tio);

	fm->port = handle;

	fm_set_baudrate(fm, FM_B19200);

	return FM_OK;
}

int
fm_set_baudrate(flowmaster *fm, fm_baud_rate rate)
{
	struct termios tio;
	
	memset(&tio, 0, sizeof(struct termios));

	tcgetattr(fm->port, &tio);

	cfsetospeed(&tio, rate);
	cfsetispeed(&tio, rate);
	
	return tcsetattr(fm->port, TCSANOW, &tio);
}

int
fm_get_baudrate(flowmaster *fm, fm_baud_rate *baud)
{
	struct termios tio;

	memset(&tio, 0, sizeof(struct termios));

	tcgetattr(fm->port, &tio);

	*baud = cfgetospeed(&tio);

	return 0;
}

fm_rc
fm_disconnect(flowmaster *fm)
{
	struct flock fl;
	/* Release the lock */

	fl.l_type = F_UNLCK;

	fcntl(fm->port, F_SETLK, &fl);

	close(fm->port);
	fm->port = 0;

	return FM_OK;
}

int
fm_isconnected(flowmaster *fm)
{
	return fm->port != 0;
}

/*
 *	Write out the buffer
 *	Returns number of bytes written.
 *	zero or negative if error
 * */
int
fm_serial_write(flowmaster *fm, int *written)
{
#if 0
	int i;
	fprintf(stdout,"%s - %d bytes\n",__func__, fm->write_buffer_len);
	
	for(i = 0; i < fm->write_buffer_len; i++){
		fprintf(stdout,"0x%02X - (%d)\n",(int)fm->write_buffer[i],(int)fm->write_buffer[i]);
	}
#endif

	ssize_t rc = write(fm->port, fm->write_buffer, fm->write_buffer_len);

	if(written != NULL){
		*written = (int) rc;
	}

	if(rc != fm->write_buffer_len){
		return -1;
	}

	return 0;
}


/*
 *	Read in from the serial port
 *	Returns zero on success
 *	nonzero on failure
 *
 *	should always read a single byte.
 * */
int
fm_serial_read_byte(flowmaster *fm, unsigned char *byte)
{
	/* TODO: use select() to make this timeout */

	int rc;
	fd_set fds;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;

	FD_ZERO(&fds);
	FD_SET(fm->port, &fds);

	rc = select(fm->port + 1, &fds, NULL, NULL, &timeout);

	if(rc == 0){
#ifdef FM_DEBUG_LOGGING
		fprintf(stderr,"Timeout waiting on port %d\n", fm->port);
#endif
		return -1;
	}

	if(FD_ISSET(fm->port, &fds)){
		rc = (int) read(fm->port, byte, 1);
		return 0;
	}

	return -1;
}

int
fm_serial_write_byte(flowmaster *fm, unsigned char byte)
{
	write(fm->port, &byte, 1);
	return 0;
}

void
fm_flush_buffers(flowmaster *fm)
{
	tcflush(fm->port, TCIOFLUSH);
}
