#ifndef __MS_NETWORK_H__
#define __MS_NETWORK_H__

#include <stdio.h>
#include <stdint.h>

#include "lwip/sockets.h"

#include "ms_commons.h"
#include "ms_crypto.h"

/*******************************************************
 *                Macros
 *******************************************************/


/*******************************************************
 *                Constants
 *******************************************************/

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
  uint64_t nonce;
  uint16_t module_id;
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
  uint64_t nonce;
  uint16_t module_id;
  uint8_t frame_type; //Per evitare endianess non uso app_frame_type_t
  app_frame_data_t data;
  uint8_t hmac[SHA256_DIGEST_LENGTH];
} app_frame_t;

typedef struct sockaddr_storage sockaddr_storage_t;

/*******************************************************
 *                Variables Declarations
 *******************************************************/

/*******************************************************
 *                Function Declarations
 *******************************************************/

int createSocket();
int bindSocket(uint16_t port);
void closeSocket();

size_t receiveUDP(uint8_t *buf, size_t bufLen, sockaddr_storage_t *source_addr);
size_t sendUDP(uint8_t *buf, size_t bufLen, const char *ip, uint16_t port);

void htonFrame(app_frame_t * frame);
void ntohFrame(app_frame_t * frame);

size_t createSensorFrame(uint64_t module_id, uint64_t nonce, float aggregate_time,
 float *sensors, uint8_t *buffer, size_t len);

#endif
