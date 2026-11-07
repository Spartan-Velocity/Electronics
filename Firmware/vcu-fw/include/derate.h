/*
 * Thermal derate for the Spartan Velocity VCU.
 *
 * A K-type thermocouple feeds a MAX31855K, which the Teensy 4.1 reads over SPI.
 * When the measured temperature goes over the trip point, the torque command
 * sent to the CM200DX inverter is scaled by a factor that decays with TIME for
 * as long as the over-temperature condition lasts, and recovers once it clears.
 *
 * This header is deliberately free of Arduino and hardware dependencies so the
 * control law can be unit tested on the host (`pio test -e native`).
 *
 * Design note: obsidian://open?vault=vault-mm&file=10-Projects%2FSpartan-Velocity%2FSubsystems%2FThermal%20Derate
 */
#ifndef DERATE_H
#define DERATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Tunable parameters. Temperatures in degrees Celsius. --- */
#define DERATE_T_TRIP_C      48.89f  /* 120 F -- derate engages above this     */
#define DERATE_T_CLEAR_C     46.11f  /* 115 F -- must fall below this to recover */
#define DERATE_TAU_DERATE_S  10.0f   /* decay time constant while over temp    */
#define DERATE_TAU_RECOVER_S 30.0f   /* recovery time constant once cleared    */
#define DERATE_F_MIN         0.30f   /* floor: never command below 30% torque  */

typedef enum {
    DERATE_NORMAL = 0,   /* at full command                      */
    DERATE_ACTIVE,       /* over temperature, factor decaying    */
    DERATE_RECOVERING,   /* cleared, factor climbing back to 1.0 */
    DERATE_FAULT         /* sensor fault: zero torque, latched   */
} derate_mode_t;

typedef struct {
    float         factor;      /* 0.0 .. 1.0, multiplies the torque command */
    derate_mode_t mode;
    float         time_over_s; /* accumulated seconds over trip, for logging */
} derate_state_t;

/* Reset to full command, no fault. */
void derate_init(derate_state_t *st);

/*
 * Advance the control law by dt_s seconds and return the new torque factor.
 *
 *   temp_c       thermocouple temperature in degrees C
 *   dt_s         elapsed time since the previous call, seconds (negative is treated as 0)
 *   sensor_fault true if the MAX31855 reported open circuit or a short
 *
 * Pure apart from the state struct: no I/O, no globals, no clock access.
 */
float derate_update(derate_state_t *st, float temp_c, float dt_s, bool sensor_fault);

/* Release a latched sensor fault. Recovery then ramps from wherever it is. */
void derate_clear_fault(derate_state_t *st);

#ifdef __cplusplus
}
#endif

#endif /* DERATE_H */
