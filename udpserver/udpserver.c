// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
	int sockfd;
	char buffer[BUF_SIZE];
	
	struct sockaddr_in servaddr, cliaddr;
	
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
	
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	
	while(1){
		
		int len, n;
		int ticks = 0;
		app_frame_t *frame;
		app_frame_type_t frameType;
		app_sensor_data_t sensorData;

		len = sizeof(cliaddr); //len is value/resuslt

		n = recvfrom(sockfd, (char *)buffer, BUF_SIZE,
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, len);
		
		if(n >= sizeof(app_frame_t)){
			frame = (app_frame_t *) buffer;
			frameType = (app_frame_type_t)frame->frame_type;

			if(frameType == SENSOR){
				sensorData = frame->data.sensor_data;

				printf("Ricevuto pacchetto SENSOR\n");
				printf("Nonce: %d, ID: %d, ");

				ticks++;
			}
		}

		if(ticks > TIME_TICKS){
			//Invio pacchetto TIME
			ticks = 0;
		}


	}
	
	return 0;
}

