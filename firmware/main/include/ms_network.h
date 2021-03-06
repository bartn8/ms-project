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
  ADV,
} app_frame_type_t;

//Implementation-dependent... Non posso usare hton... JSON???
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

typedef struct
{
  uint8_t mac[6];
} app_adv_data_t;

typedef union {
  app_sensor_data_t sensor_data;
  app_flush_data_t flush_data;
  app_time_data_t time_data;
  app_adv_data_t adv_data;
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

int receiveUDP(uint8_t *buf, size_t bufLen);
int sendUDP(uint8_t *buf, size_t bufLen, const char *ip, uint16_t port);

void htonFrameHMAC(app_frame_hmac_t *frame_hmac);
int ntohFrameHMAC(app_frame_hmac_t *frame_hmac);

size_t createSensorFrame(uint64_t module_id, float aggregate_time,
 float *sensors, uint8_t *buffer, size_t len);

size_t createAdvFrame(uint64_t module_id, uint8_t *mac, uint8_t *buffer, size_t len);

#endif
