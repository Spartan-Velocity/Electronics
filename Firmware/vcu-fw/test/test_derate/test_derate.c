/*
 * Host tests for the thermal derate law. Run with:  pio test -e native
 * These need no hardware -- that is the point of keeping derate.c pure.
 */
#include <unity.h>
#include <math.h>
#include "derate.h"

#define HOT   55.0f   /* comfortably over the 48.89 C trip  */
#define COOL  25.0f   /* comfortably under the 46.11 C clear */

static derate_state_t st;

void setUp(void)    { derate_init(&st); }
void tearDown(void) { }

/* 1. Below the trip point nothing happens at all. */
static void test_stays_full_below_trip(void)
{
    for (int i = 0; i < 100; i++) {
        derate_update(&st, COOL, 0.1f, false);
    }
    TEST_ASSERT_EQUAL_FLOAT(1.0f, st.factor);
    TEST_ASSERT_EQUAL(DERATE_NORMAL, st.mode);
}

/* 2. One time constant over temperature closes 1-1/e of the gap to the floor. */
static void test_one_tau_decay(void)
{
    const float expected = DERATE_F_MIN + (1.0f - DERATE_F_MIN) * expf(-1.0f);
    derate_update(&st, HOT, DERATE_TAU_DERATE_S, false);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, st.factor);  /* ~0.5575 */
    TEST_ASSERT_EQUAL(DERATE_ACTIVE, st.mode);
}

/* 3. Five time constants lands on the floor. */
static void test_settles_to_floor(void)
{
    for (int i = 0; i < 50; i++) {
        derate_update(&st, HOT, 1.0f, false);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, DERATE_F_MIN, st.factor);
    TEST_ASSERT_TRUE(st.factor >= DERATE_F_MIN);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, st.time_over_s);
}

/* 4. Hysteresis: between the clear and trip points, an already-derating
 *    controller keeps derating instead of chattering. */
static void test_hysteresis_holds_between_thresholds(void)
{
    derate_update(&st, HOT, 1.0f, false);
    TEST_ASSERT_EQUAL(DERATE_ACTIVE, st.mode);

    const float before = st.factor;
    derate_update(&st, 47.0f, 1.0f, false);   /* under trip, over clear */
    TEST_ASSERT_EQUAL(DERATE_ACTIVE, st.mode);
    TEST_ASSERT_TRUE(st.factor < before);

    derate_update(&st, 45.0f, 1.0f, false);   /* now under the clear point */
    TEST_ASSERT_EQUAL(DERATE_RECOVERING, st.mode);
}

/* 5. Recovery closes 1-1/e of the gap back to full in one recovery tau. */
static void test_recovery_rate(void)
{
    derate_update(&st, HOT, DERATE_TAU_DERATE_S, false);
    const float derated  = st.factor;
    const float expected = 1.0f - (1.0f - derated) * expf(-1.0f);

    derate_update(&st, COOL, DERATE_TAU_RECOVER_S, false);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, st.factor);  /* ~0.8372 */
}

/* 6. A sensor fault commands zero and latches until explicitly cleared. */
static void test_sensor_fault_latches(void)
{
    derate_update(&st, COOL, 1.0f, true);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, st.factor);
    TEST_ASSERT_EQUAL(DERATE_FAULT, st.mode);

    /* Good readings must not silently release the fault. */
    for (int i = 0; i < 20; i++) {
        derate_update(&st, COOL, 1.0f, false);
    }
    TEST_ASSERT_EQUAL_FLOAT(0.0f, st.factor);
    TEST_ASSERT_EQUAL(DERATE_FAULT, st.mode);

    derate_clear_fault(&st);
    derate_update(&st, COOL, 1.0f, false);
    TEST_ASSERT_TRUE(st.factor > 0.0f);
}

/* 7. Degenerate timesteps stay in bounds. */
static void test_timestep_edge_cases(void)
{
    derate_update(&st, HOT, 5.0f, false);
    const float before = st.factor;

    derate_update(&st, HOT, 0.0f, false);          /* no time passed */
    TEST_ASSERT_EQUAL_FLOAT(before, st.factor);

    derate_update(&st, HOT, -1.0f, false);         /* negative clamped to 0 */
    TEST_ASSERT_EQUAL_FLOAT(before, st.factor);

    derate_update(&st, HOT, 1.0e6f, false);        /* absurdly long step */
    TEST_ASSERT_TRUE(st.factor >= 0.0f && st.factor <= 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, DERATE_F_MIN, st.factor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stays_full_below_trip);
    RUN_TEST(test_one_tau_decay);
    RUN_TEST(test_settles_to_floor);
    RUN_TEST(test_hysteresis_holds_between_thresholds);
    RUN_TEST(test_recovery_rate);
    RUN_TEST(test_sensor_fault_latches);
    RUN_TEST(test_timestep_edge_cases);
    return UNITY_END();
}
