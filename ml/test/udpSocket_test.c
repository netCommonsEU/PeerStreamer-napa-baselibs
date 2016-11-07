#include<stdio.h>
#include<assert.h>
#include<malloc.h>
#ifndef _WIN32
	#include<ifaddrs.h>
#else
	#include<ws2tcpip.h>
#endif
#include<string.h>

#include "event2/util.h"

int address_family_test()
{
	printf("Testing: %s\n",__func__);
	assert(address_family("127.0.0.1")==AF_INET);
	assert(address_family("10.0.0.1")==AF_INET);
	assert(address_family("193.100.112.61")==AF_INET);
	assert(address_family("::1")==AF_INET6);
	assert(address_family("fe80::21a:a0ff:fe36:1205")==AF_INET6);
	assert(address_family("lskfjdlaskjl")==AF_UNSPEC);
	return 0;
}

int resolv_hostname_test()
{
	printf("Testing: %s\n",__func__);
	char ipaddr[200];
	resolve_hostname(ipaddr,"napasource2.disi.unitn.it",200);
	assert(strcmp(ipaddr,"193.205.213.139")==0);
	resolve_hostname(ipaddr,"193.205.213.139",200);
	assert(strcmp(ipaddr,"193.205.213.139")==0);
	resolve_hostname(ipaddr,"sldkfjalkj",200);
	assert(strcmp(ipaddr,"")==0);
	return 0;
}

int init_sockaddr_test()
{
	printf("Testing: %s\n",__func__);
	int ret;
	struct sockaddr_storage addr;
	char ipaddr[200];

	ret =	init_sockaddr(&addr,2000,"192.168.0.24");
	assert(get_sockaddr_port(&addr)==2000);
	get_sockaddr_ip(&addr,ipaddr,200);
	assert(strcmp(ipaddr,"192.168.0.24") == 0);
	assert(ret > 0);

  ret =	init_sockaddr(&addr,2000,"2001:db8::1");
	assert(get_sockaddr_port(&addr)==2000);
	get_sockaddr_ip(&addr,ipaddr,200);
	assert(strcmp(ipaddr,"2001:db8::1") == 0);
	assert(ret > 0);

	ret = init_sockaddr(&addr,-1,"sldfjls");
	assert(get_sockaddr_port(&addr)==-1);
	get_sockaddr_ip(&addr,ipaddr,200);
	assert (ret == 0);
}

int main(int argc,char** argv)
{
	printf("Hello! Starting suite test for udpSocket module\n");
	address_family_test();
	resolv_hostname_test();
	init_sockaddr_test();
	return 0;
}
