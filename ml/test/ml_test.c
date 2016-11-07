#include<assert.h>
#include<stdio.h>
#include<string.h>

#include"ml.h"

#define NH_ML_INIT_TIMEOUT {1, 0}
unsigned char cb_sign = 0;

/**
* Callback used by ML to confirm its initialization. Create a valid self nodeID and register to receive data from remote peers.
*/
static void init_myNodeID_cb_test(socketID_handle local_socketID,int errorstatus)
{
	assert(errorstatus == 0);
	cb_sign = 1;
}

void mlInit_test()
{
	printf("Testing: %s\n",__func__);
	struct timeval timeout_value = NH_ML_INIT_TIMEOUT;
	int fd;
	struct event_base* base;
	base = event_base_new();

	fd = mlInit(true,timeout_value,6666,"192.168.0.1",3478,NULL,&init_myNodeID_cb_test,base);
	assert(fd >= 0);
	while (!cb_sign) 	sleep(1);
	close(fd);

	fd = mlInit(true,timeout_value,6666,"::1",3478,NULL,&init_myNodeID_cb_test,base);
	assert(fd >= 0);
	while (!cb_sign) 	sleep(1);
	close(fd);

}


int main(int argc,char** argv)
{
	printf("Testing Messaging Layer module\n");
	mlInit_test();
	return 0;
}
