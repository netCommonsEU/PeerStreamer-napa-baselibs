#include<stdio.h>
#include<assert.h>
#include<execinfo.h>
#include<malloc.h>

#include "net_helper.h"

int net_helper_init_test()
{
	printf("Testing: %s\n",__func__);
	int res = 0;
	struct nodeID* node;
	node = net_helper_init("::1",6666,NULL);
	assert(node != NULL);
	
	free(node);
	return res;
}


int main(int argc,char** argv)
{
	printf("Hello! Starting suite test for ML Net Helper\n");
//	net_helper_init_test();
	net_helper_init_test();

	return 0;
}
