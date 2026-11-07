/*
 * Spartan Velocity VCU -- thermal derate loop.
 *
 * Signal chain, per the vcb-mk2 schematic:
 *   thermocouple T+/T-  ->  MAX31855K (U1)  ->  SPI: SO, SCK, TEMP_CS  ->  Teensy 4.1
 *   torque command      ->  CTX/CRX  ->  SN65HVD230 (U5)  ->  CAN+/CAN-  ->  CM200DX
 *
 * The control law itself lives in derate.c and is deliberately hardware-free so
 * it can be tested on the host. This file is only plumbing.
 *
 * Schematic: obsidian://open?vault=vault-mm&file=10-Projects%2FSpartan-Velocity%2FSubsystems%2FVCU
 */
#include <Arduino.h>
#include <SPI.h>
#include "derate.h"

/* ---------------------------------------------------------------------------
 * TODO(hardware): confirm these against the vcb-mk2 schematic before flashing.
 * The net labels (TEMP_CS, SO, SCK) are known; the Teensy pin numbers they land
 * on are not yet recorded anywhere, so these are the defaults, not the truth.
 * ------------------------------------------------------------------------- */
static const uint8_t PIN_TEMP_CS = 10;   /* net TEMP_CS */
/* SO -> Teensy MISO (pin 12), SCK -> Teensy SCK (pin 13), both handled by SPI */

/*
 * The MAX31855 needs 70 ms typical / 100 ms maximum per conversion, so there is
 * no point sampling faster than ~10 Hz -- a faster read just re-reads the same
 * conversion. The CAN command runs faster and holds the last factor between
 * samples, because most inverters fault out if the command message stops.
 */
static const uint32_t SENSOR_PERIOD_MS = 100;  /*  10 Hz, matched to the ADC */
static const uint32_t CAN_PERIOD_MS    = 10;   /* 100 Hz command rate        */

/* MAX31855 fault bits within the 32-bit frame */
static const uint32_t MAX31855_FAULT = 1UL << 16;
static const uint32_t MAX31855_SCV   = 1UL << 2;   /* short to Vcc */
static const uint32_t MAX31855_SCG   = 1UL << 1;   /* short to GND */
static const uint32_t MAX31855_OC    = 1UL << 0;   /* open circuit */

static derate_state_t g_derate;

struct ThermoReading {
    float    tc_c;         /* thermocouple temperature, degrees C */
    float    internal_c;   /* cold-junction temperature, degrees C */
    bool     fault;
    uint32_t raw;
};

/*
 * The MAX31855 is read-only: 32 clocks out, no command byte.
 *   bits 31..18  thermocouple temperature, 14-bit signed, 0.25 C/LSB
 *   bit  16      fault
 *   bits 15..4   internal temperature, 12-bit signed, 0.0625 C/LSB
 *   bits 2,1,0   SCV, SCG, OC
 */
static ThermoReading read_thermocouple()
{
    ThermoReading r{};
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWriteFast(PIN_TEMP_CS, LOW);
    delayMicroseconds(1);
    uint32_t raw = 0;
    for (int i = 0; i < 4; i++) {
        raw = (raw << 8) | SPI.transfer(0x00);
    }
    digitalWriteFast(PIN_TEMP_CS, HIGH);
    SPI.endTransaction();
    r.raw = raw;
    r.fault = (raw & MAX31855_FAULT) != 0;
    /* Arithmetic right shift on the signed top half sign-extends the 14-bit
     * two's complement value for free. */
    const int16_t hi = static_cast<int16_t>(raw >> 16);
    r.tc_c = static_cast<float>(hi >> 2) * 0.25f;
    const int16_t lo = static_cast<int16_t>(raw & 0xFFFF);
    r.internal_c = static_cast<float>(lo >> 4) * 0.0625f;

    /* A frame of all zeros or all ones means nothing is talking to us. */
    if (raw == 0x00000000UL || raw == 0xFFFFFFFFUL) {
        r.fault = true;
    }
    return r;
}

/* ---------------------------------------------------------------------------
 * TODO(CAN): NOT IMPLEMENTED ON PURPOSE.
 *
 * The CM200DX command message -- its arbitration ID, byte layout, torque
 * scaling and inverter-enable bits -- could not be confirmed from any public
 * source. Guessing here would mean sending an unknown command to a 200 A
 * inverter attached to a vehicle.
 *
 * Fill this in from the Cascadia Motion CAN protocol document or the DBC file,
 * then delete this comment. Until then this function only logs.
 * ------------------------------------------------------------------------- */
static void send_torque_command(float torque_nm, float factor)
{
    const float commanded = torque_nm * factor;
    (void)commanded;
    /* FlexCAN_T4 setup and the command frame go here. */
}

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_TEMP_CS, OUTPUT);
    digitalWriteFast(PIN_TEMP_CS, HIGH);
    SPI.begin();

    derate_init(&g_derate);

    /* The MAX31855 needs ~200 ms after power-up for its first conversion. */
    delay(250);
}

void loop()
{
    static uint32_t last_sensor_ms = 0;
    static uint32_t last_can_ms    = 0;
    static uint32_t last_log_ms    = 0;
    static float    factor         = 1.0f;   /* held between sensor samples */
    static ThermoReading last{};
    const uint32_t now = millis();
    /* --- control update, paced by the sensor --- */
    if (now - last_sensor_ms >= SENSOR_PERIOD_MS) {
        const float dt_s = (last_sensor_ms == 0)
                             ? (SENSOR_PERIOD_MS / 1000.0f)
                             : (now - last_sensor_ms) / 1000.0f;
        last_sensor_ms = now;

        last   = read_thermocouple();
        factor = derate_update(&g_derate, last.tc_c, dt_s, last.fault);
    }
    /* --- command output, faster and independent of the sensor --- */
    if (now - last_can_ms >= CAN_PERIOD_MS) {
        last_can_ms = now;
        /* TODO(input): real torque request, from the throttle or the CAN bus. */
        const float torque_request_nm = 0.0f;
        send_torque_command(torque_request_nm, factor);
    }
    if (now - last_log_ms >= 500) {
        last_log_ms = now;
        Serial.printf("T=%.2fC  cj=%.2fC  factor=%.3f  mode=%d%s\n",
                      last.tc_c, last.internal_c, factor, (int)g_derate.mode,
                      last.fault ? "  FAULT" : "");
    }
}
