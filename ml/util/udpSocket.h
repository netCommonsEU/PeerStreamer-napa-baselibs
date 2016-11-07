/*
 * Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *          Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *
 * This file is part of the Messaging Library.
 *
 * The Messaging Library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The Messaging Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Messaging Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file udpSocket.h 
 * @brief This file contains a udp socket abstraction. 
 *
 * The udpSocket is in the current implementation an abstraction for the linux operating system and developed with ubuntu version 8.04. Should it be neccassy to run the messaging layer under another OS the udpSocket would have to be replaced by an implementation for that operating system. 
 *
 * @author Kristian Beckers  <beckers@nw.neclab.eu> 
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu>
 * 
 * @date 7/28/2009
 */



#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <event2/event.h>
#include <time.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#else

#include <winsock2.h>

struct iovec {
  void *iov_base;
  size_t iov_len;
};

#endif

/**
 * The maximum buffer size for a send or received packet.  
 * The value is set to the maximal pmtu size.  
 */
#define MAXBUF 2000

/// @{
/**
  * socket error variable: No error occurred  
  */
#define SO_EE_ORIGIN_NONE       0

/**
  * Socket error variable: local error
  */
#define SO_EE_ORIGIN_LOCAL      1

/**
  * Socket error variable: icmp message received  
  */
#define SO_EE_ORIGIN_ICMP       2

/**
  * Socket error variable: icmp version 6 message received  
  */
#define SO_EE_ORIGIN_ICMP6      3
/// @}

#define ADDRESS_STR_LEN(sockaddr) ((((struct sockaddr_storage*)sockaddr)->ss_family == AF_INET) ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN)

typedef enum {OK = 0, MSGLEN, FAILURE, THROTTLE} error_codes;

/** 
 * A callback functions for received pmtu errors (icmp packets type 3 code 4) 
 * @param buf TODO
 * @param bufsize TODO
 */
typedef void(*icmp_error_cb)(char *buf,int bufsize);

/**
* Initialize a sockaddr_storage structure with an IPv4 or IPv6 address
*/
int init_sockaddr(struct sockaddr_storage* addr,const int port, const char *ipaddr);


int get_sockaddr_port(const struct sockaddr_storage* addr);

void resolve_hostname(char* ipaddr,const char *hostname,int str_maxlen);

void get_sockaddr_ip(const struct sockaddr_storage* addr, char* dst, size_t str_maxlen);

/** 
 * Create a messaging layer socket 
 * @param port The port of the socket
 * @param ipaddr The ip address of the socket. If left NULL the socket is bound to all local interfaces (INADDR_ANY).
 * @return The udpSocket file descriptor that the OS assigned. <0 on error
 */
int createSocket(const int port,const char *ipaddr);

/** 
 * A function to get the standard TTL from the operating system. 
 * @param udpSocket The file descriptor of the udpSocket. 
 * @param *ttl A pointer to an int that is used to store the ttl information. 
 * @return 1 if the operation is successful and 0 if a mistake occured. 
 */
int getTTL(const int udpSocket,uint8_t *ttl);

/** 
 * Sends a udp packet.
 * @param udpSocket The udpSocket file descriptor.
 * @param *buffer A pointer to the send buffer. 
 * @param bufferSize The size of the send buffer. 
 * @param *socketaddr The address of the remote socket
 */
int sendPacketFinal(const int udpSocket, struct iovec *iov, int len, struct sockaddr_storage *socketaddr);


/**
 * Receive a udp packet
 * @param udpSocket The udpSocket file descriptor.
 * @param *buffer A pointer to the receive buffer.
 * @param *recvSize A pointer to an int that is used to store the size of the recv buffer. 
 * @param *udpdst The socket address from the remote socket that send the packet.
 * @param icmpcb_value A function pointer to a callback function from type icmp_error_cb.   
 * @param *ttl A pointer to an int that is used to store the ttl information.
 */
void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_storage *udpdst,icmp_error_cb icmpcb_value,int *ttl);

/** 
 * This function is used for Error Handling. If an icmp packet is received it processes the packet. 
 * @param udpSocket The udpSocket file descriptor
 * @param iofunc An integer that states if the function was called from sendPacket or recvPaket. The value for sendPacket is 1 and for recvPacket 2.
 * @param *buf A pointer to the send or receive buffer
 * @param *bufsize A pointer to an int that ha the size of buf
 * @param *addr The address of the remote socket
 * @param icmpcb_value A function pointer to a callback function from type icmp_error_cb
 * @param *ttl A pointer to an int that is used to store the ttl information
 * @return 0 if everything went well and -1 if an error occured
 */
int handleSocketError(const int udpSocket,const int iofunc,char *buf,int *bufsize,struct sockaddr_storage *addr,icmp_error_cb icmpcb_value,int *ttl);

/** 
 * This function closes a socket  
 * @param udpSocket The file descriptor of the udp socket 
 * @return 0 if everything went well and -1 if an error occured 
 */
int closeSocket(const int udpSocket);

/** 
  * Decide if a packet should be throttled
  * The implementation follows a leaky bucket algorithm: 
  * if the packet would fill the bucket beyond its limit, it is to be discarded
  *
  * @param len The length of the packet to be sent
  * @return OK or THROTTLE 
*/
int outputRateControl(int len);

/**
  * Configure the parameters for output rate control.
  * These values may also be set while packets are being transmitted.
  * @param bucketsize The size of the bucket in bytes
  * @param drainrate The darining rate in bits/s. If drainrate is 0, then rateControl is completely disabled (all packets are passed).
*/
void setOutputRateParams(int bucketsize, int drainrate);



#endif
