# vcu-fw

Firmware for the Spartan Velocity VCU (`vcb-mk2`, Teensy 4.1).

Design notes live in the vault:

- [VCU subsystem](obsidian://open?vault=vault-mm&file=10-Projects%2FSpartan-Velocity%2FSubsystems%2FVCU)
- [Thermal derate](obsidian://open?vault=vault-mm&file=10-Projects%2FSpartan-Velocity%2FSubsystems%2FThermal%20Derate)

## Build and test

```
pio test -e native      # control law unit tests, no hardware
pio run  -e teensy41    # build for the board
```

`pio` is at `~/.platformio/penv/bin/pio`. The first `teensy41` build downloads the
platform and ARM toolchain, so it needs internet.

## Layout

| Path | What |
|---|---|
| `include/derate.h` | Control law API and tunable constants |
| `src/derate.c` | The law itself: pure C, no hardware dependencies |
| `src/main.cpp` | SPI read of the MAX31855, loop timing, CAN plumbing |
| `test/test_derate/` | Host unit tests |

## Unresolved

- **Teensy pin numbers** for `TEMP_CS`, `SO`, `SCK` are defaults, not confirmed
  against the schematic.
- **The CM200DX CAN command message is not implemented.** Its ID, byte layout,
  torque scaling and enable bits need the Cascadia protocol document. `send_torque_command()`
  is a stub rather than a guess.
- Torque request input is hardcoded to zero pending a throttle or CAN source.
