---
type: subsystem
project: Spartan Velocity
subsystem: Battery and BMS
status: in-progress
hardware:
schematic:
firmware:
updated: 2026-10-17
tags:
  - control
  - ev
---
Battery & BMS


What this subsystem is responsible for, in one or two sentences.

## Design

## Discharge Testing

- Tested a 24V battery pack through a resistive load bank
- Approximately 70A discharge (R ~ 0.34 ohm)
- Purpose: characterize cell performance under load

## BMS Configuration

- Parameter tuning only via proprietary software
- Configured: voltage/current thresholds, temperature limits, SOC drift, fault thresholds
- Did NOT develop custom BMS algorithms

## Thermal Model

- Built a lumped-capacitance thermal model to project battery cell temperature
- **Status**: unfinished, hasn't been worked on in ~1 month (as of Sept 2026)

## Implementation

| Piece | Where |
|---|---|
|  |  |

## Open questions

- [ ] 

## Log

- **YYYY-MM-DD** — 
