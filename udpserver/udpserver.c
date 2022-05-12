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
	int ticks = 0;
	
	struct sockaddr_in cliaddr;
	memset(&cliaddr, 0, sizeof(cliaddr));

	socklen_t cliaddrlen = sizeof(cliaddr);
	
	int sock = createSocket();
	bindSocket(PORT);
	
	printf("Creata socket: %d\n", sock);
	
	while(1){
		int n;
				
		app_frame_hmac_t *frame_hmac;
		app_frame_t *frame;
		app_frame_type_t frameType;

		n = receiveUDP(bufferrx, BUF_SIZE, &cliaddr, &cliaddrlen);
		
		if(n >= sizeof(app_frame_hmac_t)){
			frame_hmac = (app_frame_hmac_t *) bufferrx;

			int valid = ntohFrameHMAC(frame_hmac);
			if(valid == -1){
				frame = &(frame_hmac->frame);
				frameType = (app_frame_type_t)frame->frame_type;

				if(frameType == SENSOR){
					printf("(%d) Ricevuto pacchetto SENSOR\n", ticks);
					printf("Nonce: %ld, ID: %d, Aggregate time: %f, Sensors: [", 
					frame->nonce, frame->module_id,	frame->data.sensor_data.aggregate_time);

					int i = 0;
					for(i = 0; i < BOARD_SENSORS-1; i++){
						printf("%f, ", frame->data.sensor_data.sensors[i]);
					}

					printf("%f]\n", frame->data.sensor_data.sensors[i]);

					ticks++;
				}
			}else{
				printf("HMAC non valido!\n");
			}
			
		}

		if(ticks > TIME_TICKS){
			size_t tosend = createTimeFrame(0, 0, buffertx, BUF_SIZE);
			printf("Invio del pacchetto TIME (to send: %ld)", tosend);	


			char buffer[INET_ADDRSTRLEN];
			inet_ntop( AF_INET, &cliaddr.sin_addr, buffer, sizeof( buffer ));
			printf(" address:%s, family: %d\n", buffer, cliaddr.sin_family);

			int sent = sendUDP(buffertx, tosend, &cliaddr, sizeof(cliaddr));

			printf("Sent: %d\n", sent);

			ticks = 0;
		}


	}
	
	return 0;
}

