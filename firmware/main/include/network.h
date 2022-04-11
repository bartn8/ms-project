#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdio.h>
#include <stdint.h>

#include "lwip/sockets.h"

#include "my_commons.h"
#include "crypto.h"

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
  uint64_t nonce;
  uint16_t module_id;
  int64_t timestamp_sec;
  int64_t timestamp_usec;
  uint8_t mesh_id[MESH_ID_LENGTH];
  float sensors[BOARD_SENSORS];
} app_frame_data_t;

typedef struct
{
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


#endif
