#ifndef __UDPSERVER_H__
#define __UDPSERVER_H__

#include <stdint.h>

#define BOARD_SENSORS 10
#define SHA256_DIGEST_LENGTH 32
#define PORT	 11412
#define BUF_SIZE 1024
#define TIME_TICKS 10

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
  int64_t timestamp_sec;
  int64_t timestamp_usec;
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



#endif