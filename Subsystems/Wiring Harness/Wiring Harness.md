---
type: subsystem
project: Spartan Velocity
subsystem: Wiring Harness
status: in-progress
hardware:
schematic:
firmware:
updated: 2026-10-17
tags:
  - control
  - ev
---
# Wiring Harnesses Overview

Contains 3 main wiring harnesses for each the main subsystems of the bike. Batter management system (BMS), motor controller, and Power connections.
## Design

Three Harnesses  
  
Inverter SIde harness  
	Resolver Wires 
	
	A1 COS+
	B1 COS-
	C1 SIN +
	D1 SIN-
	E1 Resolver EXC +
	F1 Resolver EXC -

  
High side relay 1 Ground —> L1
High side relay 2 Ground —> L2

Throttle 
	5V —-> K3
	Vin —> B4
	ANALOG Ground (DO NOT CONNECT TO CHASSIS) —> A4

RS-232 ←– Should be inside the harness  
	Serial-RX —> G4
	Serial-TX —> H4
	Serial-GROUND—> F4
	Serial-Program (Should be separate) —> H2 Quick Connect 

BMS side Harness  
	`Already made
	Discharge enable relay (E stop 2)
	E stop on BMS discharge wires

   
Teensy side Harness
	BMS CANBUS (+ and -)  
	Inverter CANUS (+ and -)

  
Power side harness → PWR Switch

	Pump Power and Ground (Chassis is LV ground)  	
	Teensy power and Ground  
	Inverter Power and ground
	Power + and - —> M1
	Comms switch + and - —> M2  
	Drive Enabled + and - (E stop 1) —> M3  
	Ground —> M4
	BMS power and Ground 

  

Voltage Taps Harness  
There are 59 total voltage taps.
Resistor Taps Harness  
There are 80 total wires for this.

Connectors (No Harness needs to be made)  
	Motor Temperature 1 and 2 ( 4 Wires)

## Implementation

| Piece | Where |
| ----- | ----- |
|       |       |


