
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>


#include "include/ms_aggregate.h"


//Assunzione: l'aggregazione viene fatta single-hop
//Se ricevo un pacchetto che non viene da mio figlio lo passo diretto al padre.

static aggregate_mean_t agg_table[AGGREGATE_TABLE_SIZE];
static int agg_table_size = 0;

static mesh_mac_t children_table[CHILD_TABLE_SIZE];
static int children_table_size = 0;


int push_sensors(uint16_t module_id, float *sensors){
    struct timeval tv;
    //MAX 256 items
    uint8_t index = -1;

    for(uint8_t i = 0; i < agg_table_size; i++){
        if(agg_table[i].module_id == module_id){
            index = i;
        }
    }

    if(index == -1){
        if(agg_table_size < AGGREGATE_TABLE_SIZE){
            index = agg_table_size;

            gettimeofday(&tv, NULL);

            agg_table[agg_table_size].module_id = module_id;
            agg_table[agg_table_size].start_timestamp_sec = tv.tv_sec;
            agg_table[agg_table_size].start_timestamp_usec = tv.tv_usec;
            agg_table[agg_table_size].steps = 1;

            for(uint8_t i = 0; i < BOARD_SENSORS; i++)
                agg_table[agg_table_size].sum_sensors[i] = sensors[i];

            agg_table_size++;
        }
    }else{
        agg_table[index].steps++;
        for(uint8_t i = 0; i < BOARD_SENSORS; i++)
            agg_table[index].sum_sensors[i] += sensors[i];
    }

    return index;
}

void aggregate_sensors(uint16_t module_id, float *sensors, float *delta_time, uint32_t *steps){

}

void pop_sensors_module(uint16_t module_id);

void push_child_mac(uint8_t mac[6]);
void pop_child_mac(uint8_t mac[6]);

void push_child_id(uint16_t module_id);
void pop_child_id(uint16_t module_id);
