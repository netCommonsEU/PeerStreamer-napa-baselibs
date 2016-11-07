/*
 * Copyright (c) Agnieszka Witczak & Szymon Kuc
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

/*
 *	upgraded rateControl - token bucket
 */

#include <ml_all.h>

extern struct event_base *base;
static long bucket_size = 0;
static int64_t drain_rate = 0;


static long bytes_in_bucket = 0;
struct timeval bib_then = { 0, 0};

void planFreeSpaceInBucketEvent();

void freeSpaceInBucket_cb (int fd, short event,void *arg) {

	/*struct timeval test;

	gettimeofday(&test,NULL);

	int us;

	if(test.tv_usec > (int) arg)
		us = test.tv_usec - (int) arg;
	else
		us = 1000000 + test.tv_usec - (int) arg;

	fprintf(stderr,"Event scheduled in: %d microseconds\n",us);*/
//	fprintf(stderr,"[DEBUG] Free space callback!\n");

	while((!isQueueEmpty()) && (outputRateControl(getFirstPacketSize()) == OK)) {	

//		fprintf(stderr,"[DEBUG] Pick a packet\n");
		PacketContainer* packet = takePacketToSend();

		if (packet == NULL) return;

		struct timeval now;
   		gettimeofday(&now, NULL);
		bib_then = now;

		sendPacket(packet->udpSocket, packet->iov, 4, packet->socketaddr);

#ifdef RTX
		if (!(packet->priority & NO_RTX)) addPacketRTXqueue(packet);
		else destroyPacketContainer(packet);
#else
		destroyPacketContainer(packet);
#endif

	}

//	if (isQueueEmpty()) fprintf(stderr,"[DEBUG] Tx Queue is empty.\n");
//	else fprintf(stderr,"Rate control stopped.\n");
	if (!isQueueEmpty()) planFreeSpaceInBucketEvent(getFirstPacketSize());
	
	return;
}

void planFreeSpaceInBucketEvent(int bytes) {		//plan the event for time when there will be free space in bucket (for the first packet from the TXqueue)
//	fprintf(stderr,"[DEBUG] plan free bytes: %d\n",bytes);
	struct timeval TXtime;
	struct event *ev;

	//time needed to send data = firstPacketFromQueue.size, will free space for this packet in the bucket
	TXtime.tv_sec = bytes / drain_rate; //seconds
	TXtime.tv_usec = (bytes - TXtime.tv_sec * drain_rate) * 1000000 / drain_rate; //microseconds

//	fprintf(stderr,"[DEBUG] Launch freeSpace callback\n");
//	freeSpaceInBucket_cb(0,0,NULL);
	ev = evtimer_new(base, freeSpaceInBucket_cb, NULL);
//	fprintf(stderr,"[DEBUG] TXtime: %d,%d.\n",TXtime.tv_sec,TXtime.tv_usec);
	if (ev == NULL) fprintf(stderr,"[ERROR] freeing event not created.\n");
	event_add(ev, &TXtime);
}

int queueOrSendPacket(const int udpSocket, struct iovec *iov, int len, struct sockaddr_storage *socketaddr, unsigned char priority)
{
	PacketContainer *newPacket = createPacketContainer(udpSocket,iov,len,socketaddr,priority);

	if(!(priority & HP)) {
		if (!isQueueEmpty()) {						//some packets are already waiting, "I am for sure after them"
//			fprintf(stderr,"[DEBUG] packet queued\n");
			return addPacketTXqueue(newPacket);
		}	
		else if(outputRateControl(newPacket->pktLen) != OK) {			//queue is empty, not enough space in bucket - "I will be first in the queue"
//			fprintf(stderr,"[DEBUG] planning free space\n");
			planFreeSpaceInBucketEvent(newPacket->pktLen);		//when there will be enough space in the bucket for the first packet from the queue
			return addPacketTXqueue(newPacket);
		}
	}
#ifdef RTX
	if (!(priority & NO_RTX)) addPacketRTXqueue(newPacket);
	else destroyPacketContainer(newPacket);
#else
	destroyPacketContainer(newPacket);
#endif

	return sendPacket(udpSocket, iov, 4, socketaddr);
}

void setOutputRateParams(int bucketsize, int drainrate) { //given in Bytes and Bits/s
     bucket_size = bucketsize;
     outputRateControl(0);
     drain_rate = drainrate >> 3; //now in bytes/s
}

int outputRateControl(int len) {
	struct timeval now;
	gettimeofday(&now, NULL);

	if(drain_rate <= 0) {
		bytes_in_bucket = 0;
		bib_then = now;
		return OK;
	} else {
		unsigned int leaked;
		int total_drain_secs = bytes_in_bucket / (drain_rate) + 1;

		if(now.tv_sec - bib_then.tv_sec - 1 > total_drain_secs) {
				bytes_in_bucket = 0;
		} else {
			leaked = drain_rate * (now.tv_sec - bib_then.tv_sec);
			leaked += drain_rate * (now.tv_usec - bib_then.tv_usec) / 1000000; 
			if(leaked > bytes_in_bucket) bytes_in_bucket = 0;
					else bytes_in_bucket -= leaked;
		}

		bib_then = now;
		if(bytes_in_bucket + len <= bucket_size) {
			bytes_in_bucket += len;
			return OK;
		} else {
			return THROTTLE;
		}
   }
}
