---
type: project
project: Spartan Velocity
status: in-progress
updated: 2026-10-16
tags: [ev, project]
---

# Spartan Velocity

Electric motorcycle drivetrain.

## Architecture

![[System Map Spartan Velocity.canvas]]

Orion BMS 2 → discharge control relay → CM200DX inverter → motor, with a Teensy 4.1
VCU supervising. Power is enabled through a +12 V high-side switch, gated by the drive
enable switch and the emergency shutoff.

## Subsystems

```base
filters:
  and:
    - note.project == "Spartan Velocity"
    - note.type == "subsystem"
views:
  - type: table
    name: Subsystems
    order:
      - file.name
      - status
      - updated
    sort:
      - property: updated
        direction: ASC

```

- [[VCU]] — `vcb-mk2` control board
- [[Thermal Derate]] — 120 °F torque reduction
- [[Resolver]] — position feedback, blocked

## Hardware

| | |
|---|---|
| Board | [vcb-mk2 in KiCad](file:///mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Hardware/vcb-mk2/vcb-mk2.kicad_pro) |
| Schematic | [[vcb-mk2.svg]] |
| BOM | [[vcb-mk2-bom.csv]] |
| Firmware | [vcu-fw](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw) |

## Working notes

- [[Notes on Motor]] — resolver bench testing
- [[map]] — continuity and firmware version notes
