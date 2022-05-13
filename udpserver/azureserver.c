// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "udp_network.h"
#include "udpserver.h"


/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=***REMOVED***;DeviceId=ms-sensor-1;SharedAccessKey=***REMOVED***";
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get invoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        (void)printf("The device client is connected to iothub\r\n");
    }
    else
    {
        (void)printf("The device client has been disconnected\r\n");
    }
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = MQTT_Protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    (void)printf("Creating IoTHub Device handle\r\n");
    // Create the iothub handle here
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL)
    {
        (void)printf("Failure creating IotHub device. Hint: Check your connection string.\r\n");
    }
    else
    {
        // Set any option that are necessary.
        // For available options please see the iothub_sdk_options.md documentation

        // Can not set this options in HTTP
        bool traceOn = true;
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);

        //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
        //you are URL Encoding inputs yourself.
        //ONLY valid for use with MQTT
        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);

        char buffertx[BUF_SIZE];
        char bufferrx[BUF_SIZE];
        int ticks = 0;
        
        struct sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));

        socklen_t cliaddrlen = sizeof(cliaddr);
        
        int sock = createSocket();
        bindSocket(PORT);
        
        printf("Creata socket: %d\n", sock);

        do
        {
            int n;
                    
            app_frame_hmac_t *frame_hmac;
            app_frame_t *frame;
            app_frame_type_t frameType;

            n = receiveUDP(bufferrx, BUF_SIZE, &cliaddr, &cliaddrlen);
            
            if(n >= sizeof(app_frame_hmac_t)){
                frame_hmac = (app_frame_hmac_t *) bufferrx;

                int valid = ntohFrameHMAC(frame_hmac);
                if(valid == -1){
                    frame = &(frame_hmac->frame);
                    frameType = (app_frame_type_t)frame->frame_type;

                    if(frameType == SENSOR){
                        char message[400];
                        char module_id_str[10];
                        char timestamp_str[50];
                        char aggregate_time_str[50];
                        char sensors_str[200];
                        int sensors_pointer = 0;


                        printf("(%d) Ricevuto pacchetto SENSOR\n", ticks);
                        
                        sprintf(module_id_str, "%d", frame->module_id);
                        sprintf(timestamp_str, "%ld", frame->timestamp);
                        sprintf(aggregate_time_str, "%f", frame->data.sensor_data.aggregate_time);
                        sensors_pointer += sprintf(sensors_str+sensors_pointer, "[");

                        int i = 0;
                        for(i = 0; i < BOARD_SENSORS-1; i++){
                            sensors_pointer += sprintf(sensors_str+sensors_pointer, "%f,", frame->data.sensor_data.sensors[i]);
                        }

                        sensors_pointer += sprintf(sensors_str+sensors_pointer, "%f]", frame->data.sensor_data.sensors[i]);
                        
                        sprintf(message, "{\"module_id\": %s, \"timestamp\": %s, \"aggregate_time\": %s, \"sensors\": %s}", module_id_str, timestamp_str, aggregate_time_str, sensors_str);
                        printf("%s", message);


                        // Construct the iothub message from a string or a byte array
                        message_handle = IoTHubMessage_CreateFromString(message);
                        //message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText)));

                        // Set Message property
                        
                        // (void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
                        // (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
                        (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application/json");
                        (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");
                        // (void)IoTHubMessage_SetMessageCreationTimeUtcSystemProperty(message_handle, "2020-07-01T01:00:00.346Z");
                                                
                        // Add custom properties to message
                        //(void)IoTHubMessage_SetProperty(message_handle, "module_id", "property_value");

                        (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
                        IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

                        // The message is copied to the sdk so the we can destroy it
                        IoTHubMessage_Destroy(message_handle);

                        messages_sent++;
                        ticks++;
                    }
                }else{
                    printf("HMAC non valido!\n");
                }
                
            }

            if(ticks > TIME_TICKS){
                size_t tosend = createTimeFrame(0, buffertx, BUF_SIZE);
                printf("Invio del pacchetto TIME (to send: %ld)", tosend);	


                char buffer[INET_ADDRSTRLEN];
                inet_ntop( AF_INET, &cliaddr.sin_addr, buffer, sizeof( buffer ));
                printf(" address:%s, family: %d\n", buffer, cliaddr.sin_family);

                int sent = sendUDP(buffertx, tosend, &cliaddr, sizeof(cliaddr));

                printf("Sent: %d\n", sent);

                ticks = 0;
            }


            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            //ThreadAPI_Sleep(1);

        } while (g_continueRunning);

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    printf("Press any key to continue");
    (void)getchar();

    return 0;
}