// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "udpserver.h"
#include "udp_network.h"


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
	
	struct sockaddr_in cliaddr;
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	createSocket();
	bindSocket(PORT);
	
	while(1){
		
		int n;
		int ticks = 0;
		
		app_frame_t *frame;
		app_frame_type_t frameType;

		n = receiveUDP(bufferrx, BUF_SIZE, &cliaddr);
		
		if(n >= sizeof(app_frame_t)){
			frame = (app_frame_t *) bufferrx;
			frameType = (app_frame_type_t)frame->frame_type;

			if(frameType == SENSOR){
				printf("Ricevuto pacchetto SENSOR\n");
				printf("Nonce: %ld, ID: %d, Timestamp: %ld, Aggregate time: %f, Sensors: [", 
				frame->data.sensor_data.nonce, frame->data.sensor_data.module_id, frame->data.sensor_data.timestamp_sec,
				frame->data.sensor_data.aggregate_time);

				int i = 0;
				for(i = 0; i < BOARD_SENSORS-1; i++){
					printf("%f, ", frame->data.sensor_data.sensors[i]);
				}

				printf("%f]\n", frame->data.sensor_data.sensors[i]);

				ticks++;
			}
		}

		if(ticks > TIME_TICKS){
			createTimeFrame(0, 0, buffertx, BUF_SIZE);
			sendUDP(buffertx, sizeof(app_frame_t), &cliaddr);
			ticks = 0;
		}


	}
	
	return 0;
}

