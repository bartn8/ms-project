#ifndef __UDP_NETWORK_H__
#define __UDP_NETWORK_H__

#include <stdio.h>
#include <stdint.h>

/*******************************************************
 *                Macros
 *******************************************************/


/*******************************************************
 *                Constants
 *******************************************************/

#define BOARD_SENSORS 10
#define SHA256_DIGEST_LENGTH 32


/*******************************************************
 *                Structures
 *******************************************************/

typedef enum
{
  SENSOR = 0,
  FLUSH,
  TIME,
} app_frame_type_t;

typedef struct
{
  float aggregate_time;
  float sensors[BOARD_SENSORS];
} app_sensor_data_t;

typedef struct
{

} app_flush_data_t;

typedef struct
{
  int64_t timestamp_sec;
  int64_t timestamp_usec;
} app_time_data_t;

typedef union {
  app_sensor_data_t sensor_data;
  app_flush_data_t flush_data;
  app_time_data_t time_data;
} app_frame_data_t;

typedef struct
{
  uint64_t timestamp;
  uint16_t module_id;
  uint8_t frame_type; //Per evitare endianess non uso app_frame_type_t
  app_frame_data_t data;
} app_frame_t;

typedef struct
{
  app_frame_t frame;
  uint8_t hmac[SHA256_DIGEST_LENGTH];
} app_frame_hmac_t;

/*******************************************************
 *                Variables Declarations
 *******************************************************/

/*******************************************************
 *                Function Declarations
 *******************************************************/

int createSocket();
int bindSocket(uint16_t port);
void closeSocket();

int receiveUDP(uint8_t* buf, size_t bufLen, struct sockaddr_in* cliaddr, socklen_t* addrlen);
int sendUDP(uint8_t *buf, size_t bufLen, struct sockaddr_in* dest_addr, socklen_t addrlen);

void htonFrameHMAC(app_frame_hmac_t *frame_hmac);
int ntohFrameHMAC(app_frame_hmac_t *frame_hmac);

size_t createTimeFrame(uint64_t module_id, uint8_t *buffer, size_t len);

#endif
