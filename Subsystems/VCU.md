---
type: subsystem
project: Spartan Velocity
subsystem: VCU
status: in-progress
hardware: vcb-mk2
schematic: 10-Projects/Spartan-Velocity/Hardware/exports/vcb-mk2.svg
firmware: 10-Projects/Spartan-Velocity/Firmware/vcu-fw
updated: 2026-10-16
tags:
  - ev
  - hardware
  - vcu
---

# VCU — `vcb-mk2`

Vehicle control unit for the drivetrain. A Teensy 4.1 reads a thermocouple and a
current sensor, and talks to the CM200DX inverter over CAN.

## Schematic

![[vcb-mk2.svg]]

Regenerate with `kicad-export`. Source: [vcb-mk2.kicad_sch](file:///mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Hardware/vcb-mk2/vcb-mk2.kicad_sch)
· [open the project in KiCad](file:///mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Hardware/vcb-mk2/vcb-mk2.kicad_pro)

## Signal chain

| Function      | Part                 | Nets                                      |
| ------------- | -------------------- | ----------------------------------------- |
| Temperature   | MAX31855K (U1)       | `T+`, `T-` in; `SO`, `SCK`, `TEMP_CS` out |
| Current sense | ACS712-05B (U2)      | `VIOUT`                                   |
| CAN           | SN65HVD230D (U5)     | `CTX`, `CRX` ↔ `CAN+`, `CAN-`             |
| MCU           | Teensy 4.1 (IC1)     | —                                         |
| 5 V rail      | Pololu D24V50F5 (U4) | `+12v` → `+5v`                            |
| Coolant pump  | Q1/Q3/Q4 2N2222      | `PUMP_PWM`, `Pump`                        |

Other nets present: `SDA`, `SCL`, `D+`/`D-`, `D2+`/`D2-`, `D6`, `D7`, `PWM`, `FILTER`,
rails `+12v`, `+5v`, `+3.3v`, `GND`.

The MAX31855 is read-only, so there is no MOSI in the chain — consistent with only
`SO`, `SCK` and `TEMP_CS` appearing in the schematic.

## Subsystems

- [[Thermal Derate]] — the 120 °F torque reduction loop
- [[Resolver]] — position feedback, currently not working

## Open questions

- [ ] **`U4` is drawn wrong.** Its value is `Pololu_D24v50f5` and it is clearly meant
      to be the 5 V buck module, but its `lib_id` is `SN65HVD230`, its datasheet field
      points at `sn65hvd230.pdf`, and its footprint is a 3-pin JST connector. The BOM
      inherits all of that. Harmless on the board as built, wrong on paper.
- [ ] **2N2222 symbol mismatch.** Q1, Q3 and Q4 are plain 2N2222 parts drawn with
      `Q_NPN_Darlington_EBC`. Pinout happens to match EBC, but the symbol implies a
      Darlington and the gain assumption is wrong.
- [ ] **Teensy pin assignments are not recorded anywhere.** The firmware currently
      assumes `TEMP_CS` on pin 10 with hardware SPI on 12/13. Confirm against the board
      before flashing: [main.cpp:22](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/main.cpp:22)
- [ ] **CM200DX CAN protocol unknown.** Command message ID, byte layout, torque scaling
      and enable bits all need the Cascadia protocol document or DBC. The firmware has a
      deliberate stub rather than a guess:
      [main.cpp:92](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/main.cpp:92)
- [ ] The schematic is a single flat sheet. Worth splitting into hierarchical sheets
      (power, sensing, CAN, pump) if it grows.

## Links

- Firmware: [vcu-fw](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw)
- BOM: [[vcb-mk2-bom.csv]]
- System context: [[Spartan Velocity]]
