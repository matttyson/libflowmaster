#include <stdio.h>
#include <stdlib.h>

#include "flowmaster.h"

void callback(flash_state fs, void *mydata, void *fmdata)
{
	switch(fs) {
		case FLASH_OPEN_FILE_OK:
			printf("File open\n");
			break;
		case FLASH_OPEN_FILE_ERROR:
			printf("File open\n");
			break;
		case FLASH_BLOCK_COUNT:
			printf("Blocks: %d\n",*(int*)fmdata);
			break;
		case FLASH_WRITE_BLOCK_OK:
			printf("Wrote block %d\n",*(int*)fmdata);
			break;
		case FLASH_WRITE_BLOCK_ERROR:
			printf("Error on block %d\n",*(int*)fmdata);
			break;
		case FLASH_VALIDATE_OK:
			printf("Validate OK\n");
			break;
		case FLASH_VALIDATE_ERROR:
			printf("Validation error\n");
			break;
		case FLASH_ERASE_CHIP_BEGIN:
			printf("Starting erase\n");
			break;
		case FLASH_ERASE_CHIP_OK:
			printf("Finished erase\n");
			break;
		case FLASH_UPDATE_BEGIN:
			printf("Begninning update\n");
			break;
		case FLASH_UPDATE_OK:
			printf("completed update\n");
			break;
		case FLASH_UPDATE_ERROR:
			printf("update error\n");
			break;
	}
}

int main(int argc, char *argv[])
{
	flowmaster *fm;
	int rc;

	fm = fm_create();
	
	rc = fm_connect(fm, "/dev/ch341");

	rc = flash_validate_and_program(fm,"pumpcontrol.hex", callback, NULL);

	fm_disconnect(fm);
	fm_destroy(fm);

	return rc;
}
