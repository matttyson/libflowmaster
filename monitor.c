#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "flowmaster.h"

static void print_data(flowmaster *fm);
static void wait(int sec);

int main(int argc, char *argv[])
{
	flowmaster *fm;
	int rc;
#ifdef _WIN32
	const char port[] = "COM3";
#else
	const char port[] = "/dev/ttyUSB2";
#endif

	fm = fm_create();
	rc = fm_connect(fm, port);

	if(rc != 0){
		fprintf(stderr,"Failed to open port!\n");
		fm_destroy(fm);
		return 0;
	}

	for(;;){
		rc = fm_update_status(fm);
		if(rc != FM_OK){
			fprintf(stderr,"Got RC %d\n",(int)rc);
			continue;
		}
		print_data(fm);
		wait(1);
	}

	fm_disconnect(fm);
	fm_destroy(fm);

	return 0;
}

void
print_data(flowmaster *fm)
{
	printf("Fan duty cycle:  %0.2f%%\n", fm_fan_duty_cycle(fm) * 100.0);
	printf("Pump duty cycle: %0.2f%%\n", fm_pump_duty_cycle(fm) * 100.0);
	printf("Coolant Temp: %0.2fc\n",fm_coolant_temp(fm));
	printf("Ambient Temp: %0.2fc\n",fm_ambient_temp(fm));
	printf("Fan  Speed: %d RPM\n",fm_fan_rpm(fm));
	printf("Pump Speed: %d RPM\n",fm_pump_rpm(fm));
	//printf("Flow Rate: %0.2f LPH\n",fm_flow_rate(fm));
	printf("\n");
}

void
wait(int sec)
{
#ifdef _WIN32
	Sleep(sec * 1000);
#else
	sleep(sec);
#endif
}
