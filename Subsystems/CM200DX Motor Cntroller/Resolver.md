---
type: subsystem
project: Spartan Velocity
subsystem: CM200DX Inverter
status: blocked
hardware: vcb-mk2
updated: 2026-10-16
tags:
  - ev
  - sensing
  - resolver
---

## Resolver

Rotor position feedback for the CM200DX. Currently not producing output.
- **Type**: 6-lead BRX
- **Excitation (R1/R2)**: 14 ohm
- **Secondary windings (S1/S3, S2/S4)**: 90 ohm each

## Symptom

A 10 kHz excitation is applied, but the sine and cosine windings stay silent. All six
resolver pins show continuity. Our own resolver outputs the 10 kHz excitation but the
measured amplitude runs substantially higher than calculated; Formula's resolver shows
nothing at all on the excitation return at 1.2 Vpp.

Excitation frequencies tried: 5, 10, 27 and 35 kHz.

## State

- Firmware in use is "Group 2, 2025-05-13", which does not detect the resolver.
- Continuity across all 6 pins verified.
- Cascadia has supplied additional information on excitation levels.

Detail lives in [[Notes on Motor]] and [[map]].

## Next

- [ ] Bench Formula's resolver on a function generator, independent of the inverter
- [ ] Confirm the expected excitation amplitude against the Cascadia figures rather
      than the calculation
- [ ] Determine whether the inverter or the resolver is the broken half


# Recent State
Asked huyga from formula at which frequency they drive their resolver at. Formula uses a variable driving frequency to control their resolver and motor. Motor and resolver are driven at -3.2Khz to +3.2Khz. Our resolver is driven at 10 Khz while the motor is driven at 12 Khz. Purchased a new resolver from beyond motors, meaning if this resolver doesn't work its probably my fault.
