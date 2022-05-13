cmake_minimum_required (VERSION 3.7)
PROJECT(ms_project_azure_client C)

# Set Azure IoT SDK C settings
set(use_mqtt ON CACHE  BOOL "Set mqtt on" FORCE )
set(skip_samples ON CACHE  BOOL "Set slip_samples on" FORCE )
set(BUILD_TESTING OFF CACHE  BOOL "Set BUILD_TESTING off" FORCE )

# Add Azure IoT SDK C
add_subdirectory(../../azure-iot-sdk-c out)

compileAsC99()

set(iothub_project_files
    udp_network.c
    azureserver.c
    hmac_sha256/sha256.c 
    hmac_sha256/hmac_sha256.c
)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_c_files ${iothub_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(ms_project_azure_client ${iothub_project_files})

target_link_libraries(ms_project_azure_client iothub_client)
