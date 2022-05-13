#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "hmac_sha256/hmac_sha256.h"
#include "udp_network.h"

static int sockfd = 0;
static struct sockaddr_in servaddr;

static char key_aes[] = "d5ff2c84db72f9039580bf5c45cc28b5";
static char key_hmac[] = "dd1aafdf54893f1481885e2b7af5f31f";

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

int receiveUDP(uint8_t* buf, size_t bufLen, struct sockaddr_in* cliaddr, socklen_t* addrlen){
    if (sockfd > 0)
    {
        size_t n;
        
        n = recvfrom(sockfd, (char *)buf, bufLen,
                        MSG_WAITALL, ( struct sockaddr *) cliaddr, addrlen);

        return n;
    }
    return -1;
}

int sendUDP(uint8_t *buf, size_t bufLen, struct sockaddr_in* dest_addr, socklen_t addrlen){
    if(sockfd > 0){
        int sent = sendto(sockfd, buf, bufLen, 0, (struct sockaddr *)dest_addr, addrlen);
        if(sent < 0){
            perror("send failed");
        }

        return sent;
    }
    return -1;
}

void htonFrameHMAC(app_frame_hmac_t *frame_hmac)
{
    app_frame_t *frame = &(frame_hmac->frame);
    uint8_t *hmac = frame_hmac->hmac;
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

    hmac_sha256(key_hmac, strlen(key_hmac), frame, sizeof(app_frame_t), hmac, SHA256_DIGEST_LENGTH);
}

int ntohFrameHMAC(app_frame_hmac_t *frame_hmac)
{
    app_frame_t *frame = &(frame_hmac->frame);
    uint8_t *hmac = frame_hmac->hmac;
    uint8_t hmac_cal[SHA256_DIGEST_LENGTH];
    app_frame_type_t frame_type = (app_frame_type_t)frame->frame_type;
    app_frame_data_t *data = &(frame->data);

    hmac_sha256(key_hmac, strlen(key_hmac), frame, sizeof(app_frame_t), hmac_cal, SHA256_DIGEST_LENGTH);

    int found = -1;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        if(hmac[i] != hmac_cal[i]){
            found = i;
            break;
        }
    }

    if (found == -1){
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

    return found;
}

size_t createTimeFrame(uint64_t module_id, uint64_t nonce, uint8_t *buffer, size_t len){
    app_frame_hmac_t frame_hmac;
    app_frame_t *frame = &(frame_hmac.frame);
    app_frame_data_t *data = &(frame->data);
    struct timeval tv;
    
    if (len < sizeof(app_frame_hmac_t))
        return -1;

    frame->module_id = module_id;
    frame->nonce = nonce;
    frame->frame_type = (uint8_t) TIME;
    
    gettimeofday(&tv, NULL);
    
    data->time_data.timestamp_sec = tv.tv_sec;
    data->time_data.timestamp_usec = tv.tv_usec;

    htonFrameHMAC(&frame_hmac);

    memcpy(buffer, &frame_hmac, sizeof(app_frame_hmac_t));
    return sizeof(app_frame_hmac_t);
 }