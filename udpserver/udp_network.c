#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int bindSocket(uint16_t port){
	// Filling server information
    memset(&servaddr, 0, sizeof(servaddr));
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
    if (sockfd > 0)
    {
        size_t n;
        socklen_t source_addr_len = sizeof(struct sockaddr_in);

        n = recvfrom(sockfd, (char *)buf, bufLen,
                        MSG_WAITALL, ( struct sockaddr *) &source_addr, &source_addr_len);

        return n;
    }
    return -1;
}

size_t sendUDP(uint8_t *buf, size_t bufLen, struct sockaddr_in * dest_addr){
    if(sockfd > 0){
        size_t sent = sendto(sockfd, buf, bufLen, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
        return sent;
    }
    return -1;
}

void htonFrame(app_frame_t *frame)
{
    app_frame_type_t frame_type = (app_frame_type_t)frame->frame_type;
    app_frame_data_t *data = &(frame->data);

    frame->nonce = htobe64(frame->nonce);
    frame->module_id = htobe16(frame->module_id);

    if(frame_type == SENSOR){
        //data->sensor_data.aggregate_time = (float)htobe32((int)data->sensor_data.aggregate_time);
    }else if (frame_type == TIME)
    {
        data->time_data.timestamp_sec = htobe64(data->time_data.timestamp_sec);
        data->time_data.timestamp_usec = htobe64(data->time_data.timestamp_usec);      
    }
}

void ntohFrame(app_frame_t *frame)
{
    app_frame_type_t frame_type = (app_frame_type_t)frame->frame_type;
    app_frame_data_t *data = &(frame->data);

    frame->nonce = be64toh(frame->nonce);
    frame->module_id = be16toh(frame->module_id);

    if(frame_type == SENSOR){
        //data->sensor_data.aggregate_time = be32toh(data->sensor_data.aggregate_time);
    }else if (frame_type == TIME)
    {
        data->time_data.timestamp_sec = be64toh(data->time_data.timestamp_sec);
        data->time_data.timestamp_usec = be64toh(data->time_data.timestamp_usec);      
    }
}

size_t createTimeFrame(uint64_t module_id, uint64_t nonce, uint8_t *buffer, size_t len){
    app_frame_t frame;
    app_frame_data_t *data;
    struct timeval tv;

    if (len < sizeof(app_frame_t))
        return -1;

    frame.module_id = module_id;
    frame.nonce = nonce;
    frame.frame_type = (uint8_t) TIME;
    data = &(frame.data);

    gettimeofday(&tv, NULL);
    
    data->time_data.timestamp_sec = tv.tv_sec;
    data->time_data.timestamp_usec = tv.tv_usec;

    htonFrame(&frame);
    //TODO
    //doHMAC(((uint8_t *)&(data->time_data)), sizeof(app_time_data_t), ((uint8_t *)&(frame.hmac)));

    memcpy(buffer, &frame, sizeof(app_frame_t));
    return sizeof(app_frame_t);
 }