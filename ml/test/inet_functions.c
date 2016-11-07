#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#endif

int main(int argc, char *argv[])
{
	int res = 1;
	char *buf = malloc(INET6_ADDRSTRLEN);
	char *ipv4 = malloc(INET_ADDRSTRLEN);
	strcpy (ipv4, "10.23.23.10");
	char *ipv6 = malloc(INET6_ADDRSTRLEN);
	ipv6 = "fe80:0000:0000:0000:0223:aeff:fe54:ea0c";
	struct sockaddr_storage s;
	res = inet_pton(AF_INET, ipv4, &((struct sockaddr_in *)&s)->sin_addr);
	if(!res)
	{
		perror ("inet");
		return res;
	}
	printf("IPv4 Address %s to network %d\n",ipv4, ((struct sockaddr_in *)&s)->sin_addr);
	res = inet_ntop(AF_INET, &((struct sockaddr_in *)&s)->sin_addr, buf, INET_ADDRSTRLEN);
	if(!res)
	{
		perror ("inet");
		return res;
	}
	printf("IPv4 Address %d printable %s\n", ((struct sockaddr_in *)&s)->sin_addr, buf);

	res = inet_pton(AF_INET6, ipv6, &((struct sockaddr_in6 *)&s)->sin6_addr);
	if(!res)
	{
		perror ("inet");
		return res;
	}
	printf("IPv6 Address %s to network %ld\n", ipv6, ((struct sockaddr_in6 *)&s)->sin6_addr);
	res = inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&s)->sin6_addr, buf, INET6_ADDRSTRLEN);
	if(!res)
	{
		perror ("inet");
		return res;
	}
	int i;
    for (i = 0; i < 16; i++)
    	printf("%02x ", (unsigned char) (((struct sockaddr_in6 *)&s)->sin6_addr.__in6_u.__u6_addr8[i]));
    printf("\n");
	return res;
}
