#ifndef __MS_AGGREGATE_H__
#define __MS_AGGREGATE_H__

#include <stdint.h>

//#include "esp_err.h"

#include "ms_commons.h"

//aggregate table e child table hanno la stessa size (Single hop assumption)
//Devo lasciarle separate perché agg table può essere vuota anche se padre ha figli(non hanno trasmesso)
//Posso anche aggregare i miei sensori (+1)
#define AGGREGATE_TABLE_SIZE CONFIG_MESH_AP_CONNECTIONS+1

typedef struct{
  uint16_t module_id;
  uint8_t mac[6];
  int64_t start_timestamp_sec;
  int64_t start_timestamp_usec;
  uint32_t steps;
  float sum_sensors[BOARD_SENSORS];
} aggregate_mean_t;

int push_sensors(uint16_t module_id, float *sensors);
int aggregate_sensors(uint16_t module_id, float *sensors, float *delta_time, uint32_t *steps);
int aggregate_n_pop_sensors(uint16_t *module_id, float *sensors, float *delta_time, uint32_t *steps);
int pop_sensors(uint16_t module_id);
int pop_sensors_by_mac(uint8_t mac[6]);

int associate_mac(uint16_t module_id, uint8_t mac[6]);
size_t get_module_ids(uint16_t *module_ids);

#endif