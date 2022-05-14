// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "udp_network.h"
#include "udpserver.h"

#if __BIG_ENDIAN__
# define htonll(x) (x)
# define ntohll(x) (x)
#else
# define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
# define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

// Driver code
int main() {
	char buffertx[BUF_SIZE];
	char bufferrx[BUF_SIZE];
	int ticks = 0;
	struct timeval tv_start, tv;
	
	struct sockaddr_in cliaddr;
	memset(&cliaddr, 0, sizeof(cliaddr));

	socklen_t cliaddrlen = sizeof(cliaddr);
	
	int sock = createSocket();
	bindSocket(PORT);
	
	printf("Creata socket: %d\n", sock);
	
	gettimeofday(&tv_start, NULL);

	while(1){
		int n;
				
		app_frame_hmac_t *frame_hmac;
		app_frame_t *frame;
		app_frame_type_t frameType;

		n = receiveUDP(bufferrx, BUF_SIZE, &cliaddr, &cliaddrlen);
		gettimeofday(&tv, NULL);
		
		if(n >= sizeof(app_frame_hmac_t)){
			frame_hmac = (app_frame_hmac_t *) bufferrx;

			int valid = ntohFrameHMAC(frame_hmac);
			if(valid == -1){
				frame = &(frame_hmac->frame);
				frameType = (app_frame_type_t)frame->frame_type;

				if(frameType == SENSOR){
					ticks++;
				}
			}
		}

		if(tv.tv_sec - tv_start.tv_sec >= 1){
			//Devo stampare i byte per secondo
			float delta = tv.tv_sec - tv_start.tv_sec + (tv.tv_usec - tv_start.tv_usec) / 1000000.0f;
			float Bps = (ticks * sizeof(app_frame_hmac_t)) / delta;
			float pps = (ticks) / delta;

			printf("%f Byte/sec, %f Packet/sec\n", Bps, pps);
			
			gettimeofday(&tv_start, NULL);
			ticks = 0;
		}
	}
	
	return 0;
}

