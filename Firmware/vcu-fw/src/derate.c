#include "derate.h"
#include <math.h>

void derate_init(derate_state_t *st)
{
    if (!st) return;
    st->factor      = 1.0f;
    st->mode        = DERATE_NORMAL;
    st->time_over_s = 0.0f;
}

void derate_clear_fault(derate_state_t *st)
{
    if (!st) return;
    if (st->mode == DERATE_FAULT) {
        st->mode = DERATE_RECOVERING;
    }
}

float derate_update(derate_state_t *st, float temp_c, float dt_s, bool sensor_fault)
{
    if (!st) return 0.0f;
    if (dt_s < 0.0f) dt_s = 0.0f;

    /* A bad sensor reading is a safety fault, not a reason to guess. Command
     * zero and latch until something explicitly clears it. */
    if (sensor_fault) {
        st->mode   = DERATE_FAULT;
        st->factor = 0.0f;
        return st->factor;
    }
    if (st->mode == DERATE_FAULT) {
        return st->factor;
    }

    /* Hysteresis: once derating, stay derating until we fall below the clear
     * point, so a reading sitting on the trip point cannot chatter. */
    const bool over = (st->mode == DERATE_ACTIVE)
                        ? (temp_c > DERATE_T_CLEAR_C)
                        : (temp_c > DERATE_T_TRIP_C);

    if (over) {
        st->mode         = DERATE_ACTIVE;
        st->time_over_s += dt_s;
        /* factor -> DERATE_F_MIN, time constant DERATE_TAU_DERATE_S.
         * Incremental form: no absolute timestamp, stable for any dt. */
        st->factor = DERATE_F_MIN
                   + (st->factor - DERATE_F_MIN) * expf(-dt_s / DERATE_TAU_DERATE_S);
    } else {
        st->time_over_s = 0.0f;
        if (st->factor < 1.0f) {
            st->mode = DERATE_RECOVERING;
            /* factor -> 1.0, time constant DERATE_TAU_RECOVER_S */
            st->factor = 1.0f - (1.0f - st->factor) * expf(-dt_s / DERATE_TAU_RECOVER_S);
            if (st->factor > 0.9999f) {
                st->factor = 1.0f;
                st->mode   = DERATE_NORMAL;
            }
        } else {
            st->factor = 1.0f;
            st->mode   = DERATE_NORMAL;
        }
    }

    /* The exponential converges on its target from either side, so the only
     * clamp needed is the hard envelope. */
    if (st->factor < 0.0f) st->factor = 0.0f;
    if (st->factor > 1.0f) st->factor = 1.0f;

    return st->factor;
}
