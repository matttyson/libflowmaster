#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "flowmaster.h"

static void print_data(fm_data *data);
static void wait(int sec);

int main(int argc, char *argv[])
{
	flowmaster *fm;
	fm_data data;
	int rc;
#ifdef _WIN32
	const char port[] = "COM3";
#else
	const char port[] = "/dev/ftdi5v";
#endif

	memset(&data, 0, sizeof(fm_data));

	fm = fm_create();
	rc = fm_connect(fm, port);

	for(;;){
		rc = fm_get_data(fm, &data);
		if(rc != FM_OK){
			fprintf(stderr,"Got RC %d\n",(int)rc);
		}
		print_data(&data);
		wait(1);
	}

	fm_disconnect(fm);
	fm_destroy(fm);

	return 0;
}

void
print_data(fm_data *data)
{
	printf("Fan duty cycle:  %0.2f%%\n",data->fan_duty_cycle);
	printf("Pump duty cycle: %0.2f%%\n", data->pump_duty_cycle);
	printf("Coolant Temp: %0.2fc\n",data->coolant_temp);
	printf("Ambient Temp: %0.2fc\n",data->ambient_temp);
	printf("Fan  Speed: %d RPM\n",data->fan_rpm);
	printf("Pump Speed: %d RPM\n",data->pump_rpm);
	printf("Flow Rate: %0.2f LPH\n",data->flow_rate);
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
