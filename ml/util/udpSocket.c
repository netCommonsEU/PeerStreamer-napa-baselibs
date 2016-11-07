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

#include "../ml_all.h"

#ifdef __linux__
#include <linux/types.h>
#include <linux/errqueue.h>
#include <linux/if.h>
#include <ifaddrs.h>
#else
#define MSG_ERRQUEUE 0
#endif

#ifndef _WIN32
#include <netdb.h>
#else
	#include <ws2tcpip.h>
	#include "inet_functions.h"
#endif


/* debug varible: set to 1 if you want debug output  */
int verbose = 0;

void resolve_hostname(char* ipaddr,const char *hostname,int str_maxlen)
{
	struct addrinfo hints, *result;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;

	if(getaddrinfo(hostname,NULL,&hints,&result)) {
		ipaddr[0] = '\0';
	} else {
		get_sockaddr_ip((struct sockaddr_storage*)result->ai_addr,ipaddr,str_maxlen);
		freeaddrinfo(result);
	}
}

int address_family(const char* ipaddr)
{
	int af = AF_UNSPEC;
	struct addrinfo hints, *result;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;

	if (!getaddrinfo(ipaddr,NULL,&hints,&result))
	{
		af = result->ai_family;
		freeaddrinfo(result);
	}
	return af;
}

int init_sockaddr(struct sockaddr_storage* addr,const int port, const char *ipaddr)
{
	int inet_status = 1;
	
	addr->ss_family = address_family(ipaddr);

//	fprintf(stderr,"Initializating socket for AF: %d, ip addr: %s and port: %d\n",addr->ss_family,ipaddr,port);
//	fprintf(stderr,"Detected AF: %d for addr %s\n",addr->ss_family,ipaddr);	
	switch (addr->ss_family)
	{
		case AF_INET:
			((struct sockaddr_in*) addr)->sin_port = htons(port);
			if(ipaddr!=NULL)
				inet_status = inet_pton (addr->ss_family,ipaddr, &(((struct sockaddr_in*)addr)->sin_addr));
			else
				inet_status = inet_pton (addr->ss_family,INADDR_ANY, &(((struct sockaddr_in*)addr)->sin_addr));
#ifdef MAC_OS
			addr->ss_len = sizeof(struct sockaddr_in);
#endif
			break;
		case AF_INET6:
			((struct sockaddr_in6*) addr)->sin6_port = htons(port);
			if(ipaddr!=NULL)
				inet_status = inet_pton (addr->ss_family,ipaddr, &(((struct sockaddr_in6*)addr)->sin6_addr));
			else
				((struct sockaddr_in6*)addr)->sin6_addr = in6addr_any;
//				inet_status = inet_pton (addr->ss_family,in6addr_any, &(((struct sockaddr_in6*)addr)->sin6_addr));
#ifdef MAC_OS
			addr->ss_len = sizeof(struct sockaddr_in6);
#endif
			break;
		default:
			inet_status = 0;
//			fprintf("MEGA ERROR ON ADDRESS FAMILY %d\n",address_family(ipaddr));
	}
	
	return inet_status;
}

int get_sockaddr_port(const struct sockaddr_storage* addr)
{
	if(addr->ss_family == AF_INET)
		return ntohs(((struct sockaddr_in*)addr)->sin_port);
	if(addr->ss_family == AF_INET6)
		return ntohs(((struct sockaddr_in6*)addr)->sin6_port);
	return -1;
}

void get_sockaddr_ip(const struct sockaddr_storage* addr, char* dst, size_t str_maxlen)
{
	switch(addr->ss_family) {
		case AF_INET:
			inet_ntop(AF_INET, &(((struct sockaddr_in *) addr)->sin_addr), dst, str_maxlen);
			break;
		case AF_INET6:
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) addr)->sin6_addr), dst, str_maxlen);
			break;
		default:
			dst[0] = '\0';
	}
}

int createSocket(const int port,const char *ipaddr)
{
  /* variables needed */	
  struct sockaddr_storage udpsrc;
  int yes = 1;
  int size = sizeof(int);
  int returnStatus = 0;

  bzero((char *)&udpsrc, sizeof(udpsrc));
	if(init_sockaddr(&udpsrc,port,ipaddr)<0)
	{
		fprintf(stderr,"Cannot obtain a valid inet address.\n");
		return -1;
	}

  debug("X.CreateSock %s %d\n",ipaddr, port);


  /*libevent2*/
  evutil_socket_t udpSocket;

  /* create a socket */
  udpSocket = socket(udpsrc.ss_family, SOCK_DGRAM, IPPROTO_UDP);

  //This is debug code that checks if the socket was created
  if (udpSocket < 0)
    {
      error("Could not create an UDP socket! ERRNO %d\n",errno);
      perror ("createSocket");
      return -1;
    }

  /*libevent2*/
  evutil_make_socket_nonblocking(udpSocket);

  /* bind to the socket */
  if (setsockopt(udpSocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))) //avoid some bind errors due to fast restart, this is a UDP socket
  {
	error("Could not set socket options! ERRNO %d\n",errno);
	return -1;
  }

#ifdef MAC_OS
  if(bind(udpSocket,(struct sockaddr *)&udpsrc,udpsrc.ss_len))
#else
  if(bind(udpSocket,(struct sockaddr *)&udpsrc,sizeof(udpsrc)))
#endif
  {
    error("Could not bind socketID to address! ERRNO %d\n",errno);
    return -2;
  }

#ifdef IP_MTU_DISCOVER

  int pmtuopt = IP_PMTUDISC_DO;

  /* Setting the automatic PMTU discovery function from the socket
   * This sets the DON'T FRAGMENT bit on the IP HEADER
   */
  if(setsockopt(udpSocket,IPPROTO_IP,IP_MTU_DISCOVER,&pmtuopt,size))
  {
    error("setsockopt: set IP_DONTFRAG did not work. ERRNO %d\n",errno);
  }

  /* This option writes the IP_TTL field into ancillary data */
  if(setsockopt(udpSocket,IPPROTO_IP, IP_RECVTTL, &yes,size))
  {
	error("setsockopt: cannot set RECV_TTL. ERRNO %d\n",errno);
  }

  /* This option writes received internal and external(icmp) error messages into ancillary data */

  if(setsockopt(udpSocket,IPPROTO_IP, IP_RECVERR, &yes,size))
  {
	error("setsockopt: cannot set RECV_ERROR. ERRNO %d\n",errno);
  }

  /* Increase UDP receive buffer */

#endif

#ifdef _WIN32 //TODO: verify whether this fix is needed on other OSes. Verify why this is needed on Win (libevent?)
  int intval = 64*1024;
  if(setsockopt(udpSocket,SOL_SOCKET, SO_RCVBUF, (char *) &intval,sizeof(intval)) < 0) {
	error("setsockopt: cannot set SO_RECVBUF. ERRNO %d\n",errno);
    }

  if(setsockopt(udpSocket,SOL_SOCKET, SO_SNDBUF, (char *) &intval,sizeof(intval)) < 0) {
	error("setsockopt: cannot set SO_RECVBUF. ERRNO %d\n",errno);
    }
#endif

  debug("X.CreateSock\n");
  return udpSocket;

}

#ifndef _WIN32
/* Information: read the standard TTL from a socket  */
int getTTL(const int udpSocket,uint8_t *ttl){
#ifdef MAC_OS
	return 0;
#else

	unsigned int value;
	unsigned int size = sizeof(value);

	if(getsockopt(udpSocket,SOL_IP,IP_TTL,&value,&size))
	{
		error("get TTL did not work\n");
		return 0;
	}

	*ttl = value;
	if(verbose == 1) debug("TTL is %i\n",value);

	return 1;
#endif
}

int sendPacketFinal(const int udpSocket, struct iovec *iov, int len, struct sockaddr_storage *socketaddr)
{
	int error, ret;
	struct msghdr msgh;

	msgh.msg_name = socketaddr;
	msgh.msg_namelen = sizeof(struct sockaddr_storage);
	msgh.msg_iov = iov;
	msgh.msg_iovlen = len;
	msgh.msg_flags = 0;

	/* we do not send ancilliary data  */
	msgh.msg_control = NULL;
	msgh.msg_controllen = 0;

	/* send the message  */
	ret = sendmsg(udpSocket,&msgh,0);
	if (ret  < 0){
		error = errno;
		info("ML: sendmsg failed errno %d: %s\n", error, strerror(error));
		switch(error) {
			case EMSGSIZE:
				return MSGLEN;
			default:
				return FAILURE;
		}
	}
	return OK;
}

/* A general error handling function on socket operations
 * that is called when sendmsg or recvmsg report an Error
 *
 */

//This function has to deal with what to do when an icmp is received
//--check the connection array, look to which connection-establishment the icmp belongs
//--invoke a retransmission
int handleSocketError(const int udpSocket,const int iofunc,char *buf,int *bufsize,struct sockaddr_storage *addr,icmp_error_cb icmpcb_value,int *ttl){


	if(verbose == 1) debug("handle Socket error is called\n");

	/* variables  */
	struct msghdr msgh;
	struct cmsghdr *cmsg;
	struct sock_extended_err *errptr;
	struct iovec iov;
	struct sockaddr_storage sender_addr;
	int returnStatus;
	char errbuf[CMSG_SPACE(1024)];
	int recvbufsize = 1500;
	char recvbuf[recvbufsize];
	int icmp = 0;

	/* initialize recvmsg data  */
	msgh.msg_name = &sender_addr;
	msgh.msg_namelen = sizeof(struct sockaddr_storage);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_iov->iov_base = recvbuf;
	msgh.msg_iov->iov_len = recvbufsize;
	msgh.msg_flags = 0;

	//set the size of the control data
	msgh.msg_control = errbuf;
	msgh.msg_controllen = sizeof(errbuf);
	//initialize pointer
	errptr = NULL;

	/* get the error from the error que:  */
	returnStatus = recvmsg(udpSocket,&msgh,MSG_ERRQUEUE|MSG_DONTWAIT);
	if (returnStatus <= 0) {
		return -1;
	}
	for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh,cmsg)){
#ifdef __linux__
		//fill the error pointer
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVERR) {
			errptr = (struct sock_extended_err *)CMSG_DATA(cmsg);
//			if (errptr == NULL){
//			if(verbose == 1)
//				error("no acillary error data \n");
//	     return -1;
//		}
		/* check if the error originated locally */
			if (errptr->ee_origin == SO_EE_ORIGIN_LOCAL){
				if (errptr->ee_errno != EMSGSIZE) {
					if(verbose == 1)
						error("local error: %s \n", strerror(errptr->ee_errno));
				}
			}
			/* check if the error originated from an icmp message  */
			if (errptr->ee_origin == SO_EE_ORIGIN_ICMP){
				char sender_addr_str[INET6_ADDRSTRLEN];
				if(verbose == 1)
					debug("icmp error message received\n");

				int type = errptr->ee_type;
				int code = errptr->ee_code;
				icmp = 1;
				
				get_sockaddr_ip(&sender_addr,sender_addr_str,ADDRESS_STR_LEN(&sender_addr));
				warn("icmp error message from %s:%d is type: %d code %d\n",
					sender_addr_str,get_sockaddr_port(&sender_addr),
					errptr->ee_type,errptr->ee_code);


				/* raise the pmtu callback when an pmtu error occurred
				*  -> icmp message type 3 code 4 icmp
				*/

				if (type == 3 && code == 4){
					if(verbose == 1)
						debug("pmtu error message received\n");

					int mtusize = *bufsize;
					(icmpcb_value)(buf,mtusize);
				}
			}
		}
#endif
            ;
	} //end of for

	/* after the error is read from the socket error queue the
	* socket operation that was interrupeted by reading the error queue
	* has to be carried out
	*/

	//error("socketErrorHandle: before iofunc loop \n");
/* @TODO commented the next section because some errors are not rcoverable. ie error code EAGAIN means no dtata is available and even if you retry the eroor most probably persists (no dtata arrived in the mena time) causing recorsive callings and subsequent seg fault probably due to stack overflow
better error handling might be needed */
#if 0
	int transbuf;
	memcpy(&transbuf,bufsize,4);

	if(iofunc == 1) {
		sendPacket(udpSocket,buf,transbuf,addr,icmpcb_value);
		if(verbose == 1)
			error("handle socket error: packetsize %i \n ",*bufsize );
	} else {
		if(iofunc == 2 && icmp == 1){
			if(verbose == 1)
				error("handleSOcketError: recvPacket called \n ");
			recvPacket(udpSocket,buf,bufsize,addr,icmpcb_value,ttl);
		} else {
		/* this is the case the socket has just an error message not related to anything of the messaging layer */
			if(verbose == 1)
				error("handleSocketError: unrelated error \n");
			*ttl = -1;
		}
	}
#endif
	return 0;
}



void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_storage *udpdst,icmp_error_cb icmpcb_value,int *ttl)
{

	/* variables  */
	struct msghdr msgh; 
	struct cmsghdr *cmsg;
	int *ttlptr;
	int received_ttl;
	struct iovec iov;
	//struct sockaddr_in sender_addr;
	//unsigned int sender_len;
	int returnStatus;
	
	/* initialize recvmsg data  */
	msgh.msg_name = udpdst;
	msgh.msg_namelen = sizeof(struct sockaddr_storage);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_iov->iov_base = buffer;
	msgh.msg_iov->iov_len = *recvSize;
	msgh.msg_flags = 0;
	
	/*
	*  This shows the receiving of TTL within the ancillary data of recvmsg
	*/
	
	// ancilliary data buffer 
	char ttlbuf[CMSG_SPACE(sizeof(int))];
	
	//set the size of the control data
	msgh.msg_control = ttlbuf;
	msgh.msg_controllen = sizeof(ttlbuf);

	returnStatus = recvmsg(udpSocket,&msgh,0);
	msgh.msg_iov->iov_len = returnStatus;

	*recvSize = returnStatus;
	/* receive the message */
	if (returnStatus < 0) {
		if(verbose == 1) {
			error("udpSocket:recvPacket: Read the error queue \n ");
			error("recvmsg failed. errno %d \n",errno);
		}
		// TODO debug code: delete afterwards start
		if(errno == 11) {	//TODO: What is this 11, and why is it here with a NOP?
			int a;
			a++;
		};
		// end
		handleSocketError(udpSocket,2,buffer,recvSize,udpdst,icmpcb_value,ttl);

	} else {
		/* debug code  */
		if(verbose == 1)
			debug("udpSocket_recvPacket: Message received.\n");

		for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh,cmsg)) {
			if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
				ttlptr = (int *) CMSG_DATA(cmsg);
				received_ttl = *ttlptr;
				memcpy(ttl,ttlptr,4);
				if(verbose == 1)
					debug("received ttl true: %i  \n ",received_ttl);
				break;
			}
		}
	}
}


int closeSocket(int udpSocket)
{

  /*cleanup */
  close(udpSocket);
  return 0;

}

const char *mlAutodetectIPAddress() {
	static char addr[128] = "";
#ifdef __linux__

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	char iface[IFNAMSIZ] = "";
	char line[128] = "x";
	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		char *dummy = fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}
	if (iface[0] == 0) return NULL;

	struct ifaddrs *ifAddrStruct=NULL;
	if (getifaddrs(&ifAddrStruct)) return NULL;

	while (ifAddrStruct) {
		if (ifAddrStruct->ifa_addr && ifAddrStruct->ifa_addr->sa_family == AF_INET &&
			ifAddrStruct->ifa_name && !strcmp(ifAddrStruct->ifa_name, iface))  {
			void *tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			return inet_ntop(AF_INET, tmpAddrPtr, addr, 127);
		}
	ifAddrStruct=ifAddrStruct->ifa_next;
	}
#else
       {
                char namebuf[256];
                if(gethostname(namebuf, sizeof(namebuf))) {
                  perror("Error getting hostname");
                  return NULL;
                }
printf("Hostname is %s\n", namebuf);
                struct hostent *he = gethostbyname(namebuf);
                if(he == NULL) return NULL;
printf("Hostaddr: %x\n", *((unsigned int *)(he->h_addr)));
		return inet_ntop(AF_INET, he->h_addr, addr, 127);
       }
#endif
	return NULL;
}
#else  // WINDOWS

int sendPacketFinal(const int udpSocket, struct iovec *iov, int len, struct sockaddr_storage *socketaddr)
{
  char stack_buffer[1600];
  char *buf = stack_buffer;
  int i;
  int total_len = 0;
  int ret;
  for(i = 0; i < len; i++) total_len += iov[i].iov_len;

  if(outputRateControl(total_len) != OK) return THROTTLE;

  if(total_len > sizeof(stack_buffer)) {
     warn("sendPacket total_length %d requires dynamically allocated buffer", total_len);
     buf = malloc(total_len);
  }
  total_len = 0;
  for(i = 0; i < len; i++) {
     memcpy(buf+total_len, iov[i].iov_base, iov[i].iov_len);
     total_len += iov[i].iov_len;
  } 
  ret = sendto(udpSocket, buf, total_len, 0, (struct sockaddr *)socketaddr, sizeof(*socketaddr));
debug("Sent %d bytes (%d) to %d\n",ret, WSAGetLastError(), udpSocket);
  if(buf != stack_buffer) free(buf);
  return ret == total_len ? OK:FAILURE; 
}

void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_storage *udpdst,icmp_error_cb icmpcb_value,int *ttl)
{
  debug("recvPacket");
  int salen = sizeof(struct sockaddr_storage);
  int ret;
  ret = recvfrom(udpSocket, buffer, *recvSize, 0, (struct sockaddr *)udpdst, &salen);
  *recvSize = ret;
  if(ret == SOCKET_ERROR) {
      int err = WSAGetLastError();
      if(err = WSAECONNRESET) {
//           warn("RECVPACKET detected ICMP Unreachable  for %s:%d", inet_ntoa(udpdst->sin_addr), udpdst->sin_port); TODO
      }
      else {
           warn("RECVPACKET unclassified error %d", err);
      }
      *recvSize = -1;
      return;
  }
  *ttl=10;
}

int getTTL(const int udpSocket,uint8_t *ttl){
  return 64;
}

const char *mlAutodetectIPAddress() {
  return NULL;
}



#endif
