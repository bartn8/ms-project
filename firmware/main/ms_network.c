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
            ESP_LOGE(LOG_TAG, "Socket unable to bind: errno %d", errno);
            return err;
        }
    }
    else
    {
        ESP_LOGW(LOG_TAG, "UDP Socket not initialized.");
        return -1;
    }
    return ESP_OK;
}

void closeSocket(){
    if (udpSocket > 0)
        close(udpSocket);
}

size_t receiveUDP(uint8_t *buf, size_t bufLen, sockaddr_storage_t *source_addr)
{
    if (udpSocket > 0)
    {
        char ipbuff[INET_ADDRSTRLEN];
        socklen_t socklen = sizeof(sockaddr_storage_t);
        int len = recvfrom(udpSocket, buf, bufLen - 1, 0, (struct sockaddr *)source_addr, &socklen);

        //Controllo che sia il server che ha inviato il messaggio.

        inet_ntop(source_addr->ss_family, &(((struct sockaddr_in *)&source_addr)->sin_addr), ipbuff, INET_ADDRSTRLEN);

        return len;
    }
    else
    {
        ESP_LOGW(LOG_TAG, "UDP Socket not initialized.");
        return -1;
    }
}

size_t sendUDP(uint8_t *buf, size_t bufLen, const char *ip, uint16_t port)
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

void htonFrame(app_frame_t *frame)
{
    app_frame_data_t *data = &(frame->data);
    data->nonce = htobe64(data->nonce);
    data->module_id = htobe16(data->module_id);
    data->timestamp_sec = htobe64(data->timestamp_sec);
    data->timestamp_usec = htobe64(data->timestamp_usec);
}

void ntohFrame(app_frame_t *frame)
{
    app_frame_data_t *data = &(frame->data);
    data->nonce = be64toh(data->nonce);
    data->module_id = be16toh(data->module_id);
    data->timestamp_sec = be64toh(data->timestamp_sec);
    data->timestamp_usec = be64toh(data->timestamp_usec);
}

size_t createSensorPacket(uint64_t module_id, uint64_t nonce, int64_t start_timestamp_sec, int64_t start_timestamp_usec,
 float *sensors, uint8_t *buffer, size_t len)
{
    app_frame_t frame;
    app_frame_data_t *data;
    struct timeval tv;

    if (len < sizeof(app_frame_t))
        return -1;

    data = &(frame.data);

    gettimeofday(&tv, NULL);

    data->module_id = module_id;
    data->nonce = nonce;
    data->timestamp_sec = tv.tv_sec;
    data->timestamp_usec = tv.tv_usec;
    data->aggregate_time = tv.tv_sec - start_timestamp_sec + (tv.tv_usec - start_timestamp_usec) / 1000000.0f;

    memcpy(data->sensors, sensors, BOARD_SENSORS * sizeof(float));
    
    htonFrame(&frame);
    doHMAC(((uint8_t *)&(frame.data)), sizeof(app_frame_data_t), ((uint8_t *)&(frame.hmac)));

    memcpy(buffer, &frame, sizeof(app_frame_t));
    return sizeof(app_frame_t);
}