/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __MESH_MAIN_H__
#define __MESH_MAIN_H__

#include <sys/time.h>
#include "esp_err.h"

/*******************************************************
 *                Macros
 *******************************************************/


/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE (256)
#define TX_SIZE (256)

#define BOARD_SENSORS 10

#define MESH_ID_LENGTH 6
#define MESH_PWD_LENGTH 64
#define STATION_SSID_LENGTH 32
#define STATION_PWD_LENGTH 64
#define SERVER_IP_DOT_LENGTH 16

/*******************************************************
 *                Structures
 *******************************************************/
typedef struct
{
  uint64_t nonce;
  uint16_t module_id;
  int64_t timestamp_sec;
  int64_t timestamp_usec;
  uint8_t reqtype; //Per evitare endianess
  uint8_t mesh_id[MESH_ID_LENGTH];
  int8_t sensors[BOARD_SENSORS];
} app_frame_data_t;

typedef enum
{
  SENSORS = 0,
  ROOT,
  RESET_SENSOR
} app_reqtype_t;

typedef struct
{
  uint64_t nonce;
  int64_t timestamp_sec;
  int64_t timestamp_usec;
  uint16_t module_id;
  uint8_t reqtype; //Per evitare endianess
} app_request_data_t;

typedef struct
{
  app_request_data_t data;
  uint8_t hmac[HMAC_SHA256_DIGEST_SIZE];
} app_request_t;

typedef struct
{
  //ID MODULO 16 bit (2 byte)
  uint16_t module_id;
  //Canale WIFI (1 byte)
  uint8_t wifi_channel;
  //MESH SSID (6 byte)
  uint8_t mesh_id[MESH_ID_LENGTH];
  //MESH PWD (64 byte)
  char mesh_pwd[MESH_PWD_LENGTH];
  //STATION SSID (32 byte)
  char station_ssid[STATION_SSID_LENGTH];
  //STATION PWD (64 byte)
  char station_pwd[STATION_PWD_LENGTH];
  //SERVER IP dot notation XXX.XXX.XXX.XXX (16 byte)
  char server_ip[SERVER_IP_DOT_LENGTH];
  //SERVER PORT (2 byte)
  uint16_t server_port;
  //SERVER PORT (2 byte)
  uint16_t local_server_port;
  //TASK BRIDGE SERVICE DELAY MILLIS 16 bit (2 byte)
  uint16_t task_bridgeservice_delay_millis;
  //TASK MESH SERVICE DELAY MILLIS 16 bit (2 byte)
  uint16_t task_meshservice_delay_millis;
  //SEND TIMEOUT MILLIS 16 bit (2 byte)
  uint16_t mesh_send_timeout_millis;
} app_config_t;

typedef struct
{
  app_config_t config;
  uint32_t crc32;
} flash_data_t;

/*******************************************************
 *                Variables Declarations
 *******************************************************/

/*******************************************************
 *                Function Declarations
 *******************************************************/

void startDHCPC();
void stopDHCPC();

int createSocket();
int bindSocket();
void closeSocket();

size_t receiveUDP(uint8_t *buf, size_t bufLen);
size_t sendUDP(uint8_t *buf, size_t bufLen);

void htonFrame(app_frame_t * frame);
void ntohFrame(app_frame_t * frame);
void htonRequest(app_request_t * request);
void ntohRequest(app_request_t * request);

int setCurrentTime(app_request_data_t *request);

void readSensors(app_frame_t *frame);
void resetSensors();

size_t createSensorPacket(app_request_data_t *request, uint8_t *buffer, size_t len);

int sameAddress(mesh_addr_t *a, mesh_addr_t *b);

void esp_task_meshservice(void *arg);

#endif /* __MESH_MAIN_H__ */
