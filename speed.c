
#include <stdio.h>
#include <stdlib.h>

#include "flowmaster.h"

double read_arg(const char *arg);

int main(int argc, char *argv[])
{
	flowmaster *fm;
	int rc;
	double speed = 0;
#ifdef _WIN32
	const char port[] = "COM3";
#else
	const char port[] = "/dev/ftdi5v";
#endif

	if(argc >= 2){
		speed = read_arg(argv[1]);
	}
	else {
		fprintf(stderr,"Speed not given!\n");
		return 0;
	}

	fm = fm_create();
	rc = fm_connect(fm, port);

	if(rc != FM_OK){
		fprintf(stderr,"Error opening com port!\n");
		fm_destroy(fm);
		return 0;
	}

	rc = fm_set_fan_speed(fm,speed);
	if(rc != 0){
		fprintf(stderr,"Failed to set speed: %d\n",rc);
	}
	rc = fm_set_pump_speed(fm,speed);
	if(rc != 0){
		fprintf(stderr,"Failed to set speed: %d\n",rc);
	}

	fm_disconnect(fm);
	fm_destroy(fm);

	return 0;
}



double read_arg(const char *arg)
{
	int result;

	sscanf(arg,"%d",&result);

	return result / 100.0;
}
