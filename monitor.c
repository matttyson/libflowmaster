#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "flowmaster.h"

static void print_data(fm_data *data);
static void vary_cycle(flowmaster*);
static double read_arg(const char *arg);

int main(int argc, char *argv[])
{
	flowmaster *fm;
	fm_data data;
	int rc;
	const char port[] = "COM3";

	memset(&data, 0, sizeof(fm_data));

	fm = fm_create();
	rc = fm_connect(fm, port);
	/*
	rc = fm_set_fan_speed(fm,1.0);
	rc = fm_set_pump_speed(fm,1.0);
	*/
	for(;;){
		rc = fm_get_data(fm, &data);
		if(rc != FM_OK){
			fprintf(stderr,"Got RC %d\n",(int)rc);
		}
		print_data(&data);
		Sleep(1000);
	}

	fm_disconnect(fm);
	fm_destroy(fm);

	return 0;
}

void
vary_cycle(flowmaster *fm)
{
	const float cycle_min = 0.3f;
	const float cycle_max = 1.0f;
	double cycle = cycle_min;

	for(;;){
		fm_set_pump_speed(fm, cycle);
		cycle += 0.05;
		Sleep(1);
		if(cycle >= cycle_max){
			cycle = cycle_min;
		}
	}
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
	printf("Flow Rate: %d LPH\n",data->flow_rate);
	printf("\n");
}


float read_arg(const char *arg)
{
	int result;

	sscanf(arg,"%d",&result);

	return result / 100.0f;
}
