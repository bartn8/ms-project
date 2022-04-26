#ifndef __MS_AGGREGATE_H__
#define __MS_AGGREGATE_H__

#include <stdint.h>

//#include "esp_err.h"

#include "ms_commons.h"

//aggregate table e child table hanno la stessa size (Single hop assumption)
//Devo lasciarle separate perché agg table può essere vuota anche se padre ha figli(non hanno trasmesso)
#define AGGREGATE_TABLE_SIZE CONFIG_MESH_AP_CONNECTIONS
#define CHILD_TABLE_SIZE CONFIG_MESH_AP_CONNECTIONS

typedef struct{
  uint16_t module_id;
  int64_t start_timestamp_sec;
  int64_t start_timestamp_usec;
  uint32_t steps;
  float sum_sensors[BOARD_SENSORS];
} aggregate_mean_t;

typedef struct{
    uint16_t module_id;
    uint8_t mac[6];
} mesh_mac_t;

int push_sensors(uint16_t module_id, float *sensors);
void aggregate_sensors(uint16_t module_id, float *sensors, float *delta_time, uint32_t *steps);
void pop_sensors_module(uint16_t module_id);

void push_child_mac(uint8_t mac[6]);
void pop_child_mac(uint8_t mac[6]);

void push_child_id(uint16_t module_id);
void pop_child_id(uint16_t module_id);

#endif