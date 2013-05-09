#include <stdio.h>
#include <stdlib.h>

#include "flowmaster.h"



int main(int argc, char *argv[])
{
	flowmaster *fm;
	int rc;

	fm = fm_create();
	
	rc = fm_connect(fm, "/dev/ftdi5v");

	rc = flash_validate_and_program(fm,"pumpcontrol.hex");

	fm_disconnect(fm);
	fm_destroy(fm);

	return rc;
}
