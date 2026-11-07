# Spartan Velocity - Inverter Setup Specification

2026-09-21 by @Someone

CAN-based configuration and startup sequencing for the Cascadia CM200DX inverter driving the Beyond Motors AXM2 on Spartan Velocity's control board. Scope stops at a configured, fault-monitored inverter ready to accept throttle commands, which is a separate spec.

## Scope and hardware

**Covers:** CAN-based initialization of the inverter so it reaches a configured, fault-monitored, armed state ready to accept throttle commands. That includes EEPROM parameter setup (CAN configuration and motor limits), the continuous heartbeat, fault monitoring, and the precharge/VSM-state sequencing that gates when setup and arming are allowed to happen.

**Hardware:**

| Component | Part |
| --- | --- |
| Motor | Beyond Motors AXM2, 3-phase AC axial flux |
| Inverter | Cascadia Motion CM200DX, 200-480 VDC |
| Controller | Teensy-based VCU (this firmware) |

**Explicitly out of scope** (separate specs):

- `Throttle_Controller()` - mapping the 0x0A3 analog input to a live torque/speed command
- `fail()` - what happens after a CAN ack failure or a setup failure
- The actual EEPROM peak torque/speed/current values - pending motor characterization

**Note on the RMS GUI:** its defsyms file must date-match the inverter's firmware, and a motor-only defsyms (as originally supplied for this project) won't expose the CAN/VSM parameters. This spec's read/write path goes over CAN directly (0x0C1/0x0C2) and bypasses the GUI entirely.

## CAN message reference

All IDs below are at the default CAN ID Offset (0x0A0); they shift together if that EEPROM parameter changes.

| Message | ID | Direction | Rate | Purpose |
| --- | --- | --- | --- | --- |
| Command (heartbeat) | 0x0C0 | VCU to inverter | every 10ms, state-agnostic | torque/speed command, enable, direction, dynamic torque limit override |
| Parameter Command | 0x0C1 | VCU to inverter | on demand | read/write an EEPROM or command parameter |
| Parameter Response | 0x0C2 | Inverter to VCU | reply to 0x0C1 | ack, echoes the address |
| Internal States | 0x0AA | Inverter to VCU | \~100Hz broadcast | VSM state, inverter enable/lockout, command mode |
| Fault Codes | 0x0AB | Inverter to VCU | \~100Hz broadcast | POST and Run fault words |
| Analog Input Voltages | 0x0A3 | Inverter to VCU | \~100Hz broadcast | throttle input, bits 0-9 (feeds the deferred `Throttle_Controller()`) |

## EEPROM parameters

Written once at setup, over 0x0C1, motor disabled, and verified by read-back after the power cycle setup requires. Loaded from a `want[]` table on the Teensy; only mismatches get written.

**CAN configuration:**

| Addr | Name | Value | Status |
| --- | --- | --- | --- |
| 143 | Inverter Command Mode | 0 (CAN mode) | set |
| 142 | Run Mode | 0 (torque mode) | set |
| 141 | CAN ID Offset | 0xA0 (default) | set |
| 147 | CAN Bit Rate | TBD | needs to match the bus |
| 146 | CAN Command Message Active | 1 | set - enables the CAN timeout fault |
| 172 | CAN Timeout | 333 (\~1s) | placeholder |
| 238 | CAN Debounce Counter Max | TBD | confirm this firmware supports the Gen5/CM rolling counter - a read returning 0,0 in bytes 0-1 means it doesn't |

**Motor and limit parameters - all TBD, pending characterization:**

| Addr | Name | Notes |
| --- | --- | --- |
| 150 | Motor Parameter Set | motor type index for the AXM2; write first, it resets dependent defaults |
| 129 | Motor Torque Limit | Nm x10 |
| 128 | Max Speed | rpm |
| 100 / 101 | Iq / Id Limit | current limits - EEPROM-only, no per-frame override available |

## Fault handling

0x0AB carries two little-endian 32-bit words: bytes 0-3 are POST faults (latch only at power-up), bytes 4-7 are Run faults. Each byte is shifted into place and OR'd together:

```c
uint32_t post = b[0] | b[1]<<8 | b[2]<<16 | (uint32_t)b[3]<<24;
uint32_t run  = b[4] | b[5]<<8 | b[6]<<16 | (uint32_t)b[7]<<24;
```

Masks are looked up in a const array of structs, not a chain of `if`s:

```c
typedef struct { uint32_t mask; const char *name; } Fault;
```

**Masks captured so far:**

| Word | Mask | Name |
| --- | --- | --- |
| POST | 0x00000001 | HW gate/desat |
| Run | 0x00000100 | HW gate/desat |
| Run | 0x40000000 | Resolver not connected |

More get added as they're hit on the bench; an unmatched-but-nonzero word should print the raw hex rather than stay silent.

The `waiting` flag is the only thing shared between the send and receive sides: the setup step sets it to 1 when a 0x0C1 goes out, and either the matching 0x0C2 or the 100ms timeout clears it back to 0. Only one request is ever in flight.

The EEPROM setup sequence itself is a separate, smaller state machine:

## Heartbeat (0x0C0)

State-agnostic: sent unconditionally every 10ms from before the inverter powers up until shutdown, on its own clock, independent of `vsm`, `error_flag`, or setup progress. If it stops, the inverter faults on CAN Command Message Lost. Only the payload changes with state - `enable` stays 0 until armed.

| Bytes | Field | Notes |
| --- | --- | --- |
| 0-1 | Torque Command | Nm x10 |
| 2-3 | Speed Command | rpm; nonzero overrides Max Speed live, even in torque mode |
| 4 | Direction | 0 = reverse, 1 = forward |
| 5, bit 0 | Inverter Enable | gated on `setup_state == S_DONE && vsm == 5 && !error_flag` |
| 5, bit 1 | Inverter Discharge |  |
| 5, bit 2 | Speed Mode Enable | one-way override, torque to speed only |
| 5, bits 4-7 | Rolling Counter | Gen5/CM only, see below |
| 6-7 | Commanded Torque Limit | 0 = use the EEPROM default; nonzero overrides the Motor/Regen torque limit for that frame - this is how a live, speed-dependent torque ceiling gets applied, with no EEPROM write |

**Rolling counter:** the CM200DX expects an incrementing 0-15 count in byte 5 bits 4-7. A constant value faults within a few frames under the default debounce settings (max 20, +5 per bad frame, -3 per good one). Increment it every send, or set EEPROM addr 238 to 0 to disable the check - confirm on the bench which firmware this unit runs.

## Parameter read/write protocol (0x0C1/0x0C2)

| Bytes | 0x0C1 (request) | 0x0C2 (reply) |
| --- | --- | --- |
| 0-1 | Address, little-endian | Address, echoed - 0,0 means unrecognized |
| 2 | 1 = write, 0 = read | 1 = write succeeded (write replies only) |
| 3 | reserved | reserved |
| 4-5 | Data to write | Data read or written |
| 6-7 | reserved | reserved |

Only one request is ever outstanding, gated by `waiting`:

Ack match:

```c
uint16_t reply_addr = (reply.b[0] | (reply.b[1] << 8));
int address_matches = (reply_addr == want[idx].addr);
```

## 

`canbusSend(msg)` still needs to be defined too - the thin wrapper around the MCU's CAN peripheral that every 0x0C0/0x0C1 send in this spec actually calls to queue and transmit the frame.
