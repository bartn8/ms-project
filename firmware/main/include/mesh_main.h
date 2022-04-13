/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __MESH_MAIN_H__
#define __MESH_MAIN_H__

#include "ms_commons.h"
#include "ms_network.h"
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
  //HMAC SECRET (32 byte)
  uint8_t key_hmac[SHA256_KEY_LENGTH];
  //AES SECRET (32 byte)
  uint8_t key_aes[AES_KEY_LENGTH];
  //AES-CBC IV (16 byte)
  uint8_t iv_aes[AES_BLOCK_LENGTH];
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

int setCurrentTime(int64_t timestamp_sec, int64_t timestamp_usec);

void readSensors(app_frame_t *frame);
void resetSensors();

size_t createSensorPacket(uint64_t nonce, uint8_t *buffer, size_t len);

int sameAddress(mesh_addr_t *a, mesh_addr_t *b);

void esp_task_meshservice(void *arg);

#endif /* __MESH_MAIN_H__ */
