#ifndef __MS_CRYPTO_H__
#define __MS_CRYPTO_H__

#include <stdio.h>
#include <stdint.h>

#include "ms_commons.h"

/*******************************************************
 *                Macros
 *******************************************************/


/*******************************************************
 *                Constants
 *******************************************************/

#define AES_BLOCK_LENGTH 16
#define AES_KEY_LENGTH_BITS 256
#define AES_KEY_LENGTH (AES_KEY_LENGTH_BITS/8)
#define SHA256_KEY_LENGTH 32
#define SHA256_DIGEST_LENGTH 32

/*******************************************************
 *                Structures
 *******************************************************/

/*******************************************************
 *                Variables Declarations
 *******************************************************/

/*******************************************************
 *                Function Declarations
 *******************************************************/

int initHMAC(const uint8_t *key);
int changeHMACKey(const uint8_t *key);
int doHMAC(const uint8_t *payload, size_t payloadLength, uint8_t *hmacResult);
void freeHMAC();

int initAES(const uint8_t *key, const uint8_t *iv);
size_t padding(uint8_t *payload, size_t payloadLength);
int encryptAES_CBC(const uint8_t *payload, size_t payloadLength, uint8_t *encryptResult);
int decryptAES_CBC(const uint8_t *payload, size_t payloadLength, uint8_t *decryptResult);
void freeAES();

#endif