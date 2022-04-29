
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "include/ms_aggregate.h"


//Assunzione: l'aggregazione viene fatta single-hop
//Se ricevo un pacchetto che non viene da mio figlio lo passo diretto al padre.

static aggregate_mean_t agg_table[AGGREGATE_TABLE_SIZE];
static uint8_t agg_table_size = 0;

//MAX 256 items
static void pop_sensors_index(int16_t index){
    if (index >= 0){
        if (agg_table_size > 0)
            agg_table_size--;

        //Devo shiftare a sinistra
        for(uint8_t i = (uint8_t)index; i < agg_table_size; i++){
            memcpy(agg_table+i, agg_table+i+1, sizeof(aggregate_mean_t));
        }   
    }
}

static int16_t get_sensor_index_by_id(uint16_t module_id){
    int16_t index = -1;
    for(uint8_t i = 0; i < agg_table_size; i++){
        if(agg_table[i].module_id == module_id){
            index = i;
        }
    }
    return index;
}

static int16_t get_sensor_index_by_mac(uint8_t mac[6]){
    int16_t index = -1;
    for(uint8_t i = 0; i < agg_table_size; i++){
        if(memcmp(agg_table[i].mac, mac, 6) == 0){
            index = i;
        }
    }
    return index;
}

int push_sensors(uint16_t module_id, float *sensors){
    struct timeval tv;
    //MAX 256 items
    int16_t index = get_sensor_index_by_id(module_id);

    if (index >= 0){
        agg_table[index].steps++;
        for(uint8_t i = 0; i < BOARD_SENSORS; i++)
            agg_table[index].sum_sensors[i] += sensors[i];
    }else{
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
    }

    return index;
}

int aggregate_sensors(uint16_t module_id, float *sensors, float *delta_time, uint32_t *steps){
    int16_t index = get_sensor_index_by_id(module_id);
    struct timeval tv;

    if (index >= 0){
        *delta_time = tv.tv_sec - agg_table[index].start_timestamp_sec + (tv.tv_usec - agg_table[index].start_timestamp_usec) / 1000000.0f;
        *steps = agg_table[index].steps;

        for(uint8_t i = 0; i < BOARD_SENSORS; i++)
            sensors[i] = agg_table[index].sum_sensors[i] / agg_table[index].steps;

        pop_sensors_index(index);
    }
    
    return index;
}

int aggregate_n_pop_sensors(uint16_t *module_id, float *sensors, float *delta_time, uint32_t *steps){
    int16_t index = agg_table_size-1;
    if(index>=0){
        *module_id = agg_table[index].module_id;
        *delta_time = tv.tv_sec - agg_table[index].start_timestamp_sec + (tv.tv_usec - agg_table[index].start_timestamp_usec) / 1000000.0f;
        *steps = agg_table[index].steps;

        for(uint8_t i = 0; i < BOARD_SENSORS; i++)
            sensors[i] = agg_table[index].sum_sensors[i] / agg_table[index].steps;

        pop_sensors_index(index);    
    }
    return index;
}

int pop_sensors(uint16_t module_id){
    int16_t index = get_sensor_index_by_id(module_id);
    pop_sensors_index(index);
    return index;
}

int pop_sensors_by_mac(uint8_t mac[6]){
    int16_t index = get_sensor_index_by_mac(mac);
    pop_sensors_index(index);
    return index;    
}

int associate_mac(uint16_t module_id, uint8_t mac[6]){
    int16_t index = get_sensor_index_by_id(module_id);
    if (index >= 0)
        memcpy(agg_table[index].mac, mac, 6);
    return index;
}

size_t get_module_ids(uint16_t *module_ids){
    for(uint8_t i = 0; i < agg_table_size; i++){
        module_ids[i] = agg_table[i].module_id;
    }
    return agg_table_size;
}