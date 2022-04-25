#include <string.h>
#include <stdint.h>
#include <math.h>

#include "esp_random.h"

#include "include/ms_gpio.h"

static float sensors_state[BOARD_SENSORS];

//From: https://cse.usf.edu/~kchriste/tools/gennorm.c
//===========================================================================
//=  Function to generate normally distributed random variable using the    =
//=  Box-Muller method                                                      =
//=    - Input: mean and standard deviation                                 =
//=    - Output: Returns with normally distributed random variable          =
//===========================================================================
static double norm(double mean, double std_dev)
{
    double   u, r, theta;           // Variables for Box-Muller method
    double   x;                     // Normal(0, 1) rv
    double   norm_rv;               // The adjusted normal rv

    // Generate u
    u = 0.0;
    while (u == 0.0)
        u = (double)esp_random()/UINT32_MAX;

    // Compute r
    r = sqrt(-2.0 * log(u));

    // Generate theta
    theta = 0.0;
    while (theta == 0.0)
        theta = 2.0 * M_PI * ((double)esp_random()/UINT32_MAX);

    // Generate x value
    x = r * cos(theta);

    // Adjust x value for specified mean and variance
    norm_rv = (x * std_dev) + mean;

    // Return the normally distributed RV value
    return(norm_rv);
}

void readSensors(float *sensors){
    memcpy(sensors, sensors_state, BOARD_SENSORS * sizeof(float));
}

void updateSensors(){
    //Per ora si fa un rumore gaussiano
    for (uint8_t i = 0; i < BOARD_SENSORS; i++)
    {
        sensors_state[i] = (float)norm(i, 1);
    }
}

void resetSensors(){
    memset(sensors_state, 0, BOARD_SENSORS * sizeof(float));
}