#include "include/crypto.h"

#include <stdio.h>
#include <string.h> 

//Esp libs
#include "esp_err.h"
#include "esp_log.h"

//Crypto libs
#include "mbedtls/md.h"
#include "mbedtls/aes.h"

static const char *LOG_TAG = "ms-project-network";

static mbedtls_md_context_t md_ctx;
static mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

static mbedtls_aes_context aes_ctx;
static uint8_t aes_iv[AES_BLOCK_LENGTH];

int initHMAC(const uint8_t *key)
{
    mbedtls_md_init(&md_ctx);
    int setup_rst = mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(md_type), 1);

    if (setup_rst == MBEDTLS_ERR_MD_ALLOC_FAILED)
    {
        ESP_LOGE(LOG_TAG, "HMAC init failed!");
        return MBEDTLS_ERR_MD_ALLOC_FAILED;
    }

    return mbedtls_md_hmac_starts(&md_ctx, key, SHA256_KEY_LENGTH);
}

void doHMAC(const uint8_t *payload, size_t payloadLength, uint8_t *hmacResult)
{
    mbedtls_md_hmac_reset(&md_ctx);
    mbedtls_md_hmac_update(&md_ctx, payload, payloadLength);
    mbedtls_md_hmac_finish(&md_ctx, hmacResult);
}

void freeHMAC(){
    mbedtls_md_free(&md_ctx);
}

int initAES(const uint8_t *key, const uint8_t *iv){
    memcpy(aes_iv, iv, AES_BLOCK_LENGTH);
    mbedtls_aes_init(&aes_ctx);
    return mbedtls_aes_setkey_enc(&aes_ctx, key, AES_KEY_LENGTH_BITS);
}

size_t padding(uint8_t *payload, size_t payloadLength){
    //Data copy
    size_t newPayloadLength = (payloadLength/16+1)*16;
    
    //Real padding
    for(uint8_t i = 0; i < (uint8_t)(newPayloadLength-payloadLength);i++){
        // PKCS#7
        payload[payloadLength+i] = (uint8_t)(newPayloadLength-payloadLength);
    }

    return newPayloadLength;
}

int encryptAES_CBC(const uint8_t *payload, size_t payloadLength, uint8_t *encryptResult){
    //Payload padding
    if(payloadLength % 16 != 0){
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    #if defined(MBEDTLS_CIPHER_MODE_CBC)
    uint8_t iv[AES_BLOCK_LENGTH];
    memcpy(iv, aes_iv, AES_BLOCK_LENGTH);
    return mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, payloadLength, iv, payload, encryptResult);
    #else
    uint8_t block[AES_BLOCK_LENGTH];
    uint8_t blockxor[AES_BLOCK_LENGTH];
    uint8_t block_output[AES_BLOCK_LENGTH];
    //Block iterations
    memcpy(blockxor, aes_iv, AES_BLOCK_LENGTH);
    
    for(size_t i = 0; i < payloadLength; i+=AES_BLOCK_LENGTH){
        memcpy(block, payload_ptr+i, AES_BLOCK_LENGTH);
        for(uint8_t i = 0; i < AES_BLOCK_LENGTH; i++){
            block[i] = block[i] ^ blockxor[i];
        }
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, block, block_output);
        memcpy(blockxor, block_output, AES_BLOCK_LENGTH);
        memcpy(encryptResult+i, block_output, AES_BLOCK_LENGTH);
    }
    return ESP_OK;
    #endif
}

int decryptAES_CBC(const uint8_t *payload, size_t payloadLength, uint8_t *decryptResult){
    //Payload padding
    if(payloadLength % 16 != 0){
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    #if defined(MBEDTLS_CIPHER_MODE_CBC)
    uint8_t iv[AES_BLOCK_LENGTH];
    memcpy(iv, aes_iv, AES_BLOCK_LENGTH);
    return mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, payloadLength, iv, payload, decryptResult);
    #else
    uint8_t block[AES_BLOCK_LENGTH];
    uint8_t blockxor[AES_BLOCK_LENGTH];
    uint8_t block_output[AES_BLOCK_LENGTH];
    //Block iterations    
    memcpy(blockxor, aes_iv, AES_BLOCK_LENGTH);
    
    for(size_t i = 0; i < payloadLength; i+=AES_BLOCK_LENGTH){
        memcpy(block, payload+i, AES_BLOCK_LENGTH);
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, block, block_output);
        for(uint8_t i = 0; i < AES_BLOCK_LENGTH; i++){
            block_output[i] = block_output[i] ^ blockxor[i];
        }
        memcpy(blockxor, block, AES_BLOCK_LENGTH);
        memcpy(decryptResult+i, block_output, AES_BLOCK_LENGTH);
    }
    return ESP_OK;
    #endif
}

void freeAES(){
    mbedtls_aes_free(&aes_ctx);
}