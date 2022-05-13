#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

#include "esp_err.h"
#include "esp_log.h"
#include "endian.h"

//Socket
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "include/ms_network.h"

static const char *LOG_TAG = "ms-project-network";
static int udpSocket = 0;

int createSocket()
{
    if (udpSocket > 0)
        close(udpSocket);

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (udpSocket < 0)
    {
        ESP_LOGE(LOG_TAG, "Unable to create socket: errno %d", errno);
    }

    return udpSocket;
}

int bindSocket(uint16_t port)
{
    struct sockaddr_in6 dest_addr;
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;

    if (udpSocket > 0)
    {
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(port);
        int err = bind(udpSocket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(LOG_TAG, "(BIND)Socket unable to bind: errno %d", errno);
            return err;
        }
        return ESP_OK;
    }
    else
    {
        ESP_LOGW(LOG_TAG, "(BIND)UDP Socket not initialized.");
    }
    return -1;
}

void closeSocket(){
    if (udpSocket > 0){
        close(udpSocket);
        udpSocket = 0;
    }
}

int receiveUDP(uint8_t *buf, size_t bufLen)
{
    if (udpSocket > 0)
    {
        //char ipbuff[INET_ADDRSTRLEN];
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(udpSocket, buf, bufLen - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        //Controllo che sia il server che ha inviato il messaggio.
        //inet_ntop(source_addr->ss_family, &(((struct sockaddr_in *)&source_addr)->sin_addr), ipbuff, INET_ADDRSTRLEN);

        return len;
    }
    else
    {
        ESP_LOGW(LOG_TAG, "UDP Socket not initialized.");
        return -1;
    }
}

int sendUDP(uint8_t *buf, size_t bufLen, const char *ip, uint16_t port)
{
    if (udpSocket > 0)
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htobe16(port);

        size_t sent = sendto(udpSocket, buf, bufLen, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (sent < bufLen)
            ESP_LOGE(LOG_TAG, "UDP Send failed (%d < %d)", sent, bufLen);

        return sent;
    }
    else
    {
        ESP_LOGW(LOG_TAG, "UDP Socket not initialized.");
        return -1;
    }
}

void htonFrameHMAC(app_frame_hmac_t *frame_hmac)
{
    app_frame_t *frame = &(frame_hmac->frame);
    uint8_t *hmac = frame_hmac->hmac;
    app_frame_type_t frame_type = (app_frame_type_t)frame->frame_type;
    app_frame_data_t *data = &(frame->data);

    frame->timestamp = htobe64(frame->timestamp);
    frame->module_id = htobe16(frame->module_id);

    if(frame_type == SENSOR){
        //data->sensor_data.aggregate_time = htobe32(data->sensor_data.aggregate_time);
    }else if (frame_type == TIME)
    {
        data->time_data.timestamp_sec = htobe64(data->time_data.timestamp_sec);
        data->time_data.timestamp_usec = htobe64(data->time_data.timestamp_usec);      
    }

    doHMAC((const uint8_t *)frame, sizeof(app_frame_t), hmac);
}

int ntohFrameHMAC(app_frame_hmac_t *frame_hmac)
{
    app_frame_t *frame = &(frame_hmac->frame);
    uint8_t *hmac = frame_hmac->hmac;
    uint8_t hmac_cal[SHA256_DIGEST_LENGTH];
    app_frame_type_t frame_type = (app_frame_type_t)frame->frame_type;
    app_frame_data_t *data = &(frame->data);

    doHMAC((const uint8_t*)frame, sizeof(app_frame_t), hmac_cal);

    int found = -1;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        if(hmac[i] != hmac_cal[i]){
            found = i;
            break;
        }
    }

    if(found == -1){
        frame->timestamp = be64toh(frame->timestamp);
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

size_t createSensorFrame(uint64_t module_id, float aggregate_time,
 float *sensors, uint8_t *buffer, size_t len)
{
    app_frame_hmac_t frame_hmac;
    app_frame_t *frame = &(frame_hmac.frame);
    app_frame_data_t *data = &(frame->data);
    struct timeval tv;
    
    if (len < sizeof(app_frame_hmac_t))
        return -1;

    frame->module_id = module_id;
    frame->frame_type = (uint8_t) SENSOR;

    gettimeofday(&tv, NULL);
    frame->timestamp = tv.tv_sec;

    data->sensor_data.aggregate_time = aggregate_time;
    memcpy(data->sensor_data.sensors, sensors, BOARD_SENSORS * sizeof(float));
    
    htonFrameHMAC(&frame_hmac);
    
    memcpy(buffer, &frame_hmac, sizeof(app_frame_hmac_t));
    return sizeof(app_frame_hmac_t);
}