#ifndef _inet_functions_h_
#define _inet_functions_h_ 1

#ifdef _WIN32
#include <ws2tcpip.h>
#include <winsock2.h>
#else
#include <sys/systm.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

int inet_pton(int af, const char *src, void *dst);

char * inet_ntop(int af, const void *src, char *dst, socklen_t size);

#endif
