#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "udp_network.h"

static int sockfd = 0;
static struct sockaddr_in servaddr;

int createSocket()
{
    if (sockfd > 0)
        close(sockfd);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        printf( "Unable to create socket: errno %d\n", sockfd);
    }

    return sockfd;
}

int bindSocket( uint16_t port){
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);
	
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
        return -1;
	}

    return 0;
}

void closeSocket(){
    close(sockfd);
    sockfd = 0;
}

size_t receiveUDP(uint8_t *buf, size_t bufLen, struct sockaddr_in * source_addr){
    len = sizeof(cliaddr); //len is value/resuslt

    n = recvfrom(sockfd, (char *)buf, bufLen,
					MSG_WAITALL, ( struct sockaddr *) &cliaddr, len);
}

size_t sendUDP(uint8_t *buf, size_t bufLen, const char *ip, uint16_t port);

void htonFrame(app_frame_t * frame);
void ntohFrame(app_frame_t * frame);

size_t createTimeFrame(uint64_t module_id, uint64_t nonce, int64_t timestamp_sec, int64_t timestamp_usec,
 uint8_t *buffer, size_t len);