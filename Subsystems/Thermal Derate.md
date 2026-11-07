---
type: subsystem
project: Spartan Velocity
subsystem: Thermal Derate
status: draft
hardware: "[[VCU]]"
schematic: 10-Projects/Spartan-Velocity/Hardware/exports/vcb-mk2.svg
firmware: 10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/derate.c
updated: 2026-10-16
tags: [ev, thermal, control]
---

# Thermal Derate

Protect the drivetrain by reducing commanded torque when measured temperature
goes over 120 °F, and restore it when things cool down.

## The loop

```tikz
\usepackage{tikz}
\usetikzlibrary{arrows.meta, positioning, calc}
\begin{document}
\begin{tikzpicture}[
  font=\sffamily\small,
  block/.style={draw, rounded corners=2pt, minimum height=9mm, minimum width=20mm,
                align=center, inner sep=4pt, line width=0.4pt},
  sum/.style={draw, circle, minimum size=6mm, inner sep=0pt},
  >={Stealth[length=2mm]},
  node distance=9mm
]
\node[block] (tc)   {Thermocouple\\(K-type)};
\node[block, right=of tc] (max) {MAX31855K\\SPI};
\node[block, right=of max] (trip) {Trip detect\\+ hysteresis};
\node[block, right=of trip] (decay) {Time decay\\$f(t)$};
\node[sum, right=14mm of decay] (mul) {$\times$};
\node[block, right=10mm of mul] (can) {CAN\\SN65HVD230};
\node[block, right=of can] (inv) {CM200DX\\inverter};
\node[block, below=12mm of inv] (motor) {Motor};

\node[above=8mm of mul, align=center] (req) {torque\\request};

\draw[->] (tc)   -- node[above,font=\tiny]{mV} (max);
\draw[->] (max)  -- node[above,font=\tiny]{$T$} (trip);
\draw[->] (trip) -- node[above,font=\tiny]{over?} (decay);
\draw[->] (decay) -- node[above,font=\tiny]{$f$} (mul);
\draw[->] (req)  -- (mul);
\draw[->] (mul)  -- (can);
\draw[->] (can)  -- (inv);
\draw[->] (inv)  -- (motor);

% thermal feedback: the motor is what heats the thermocouple
\draw[->, dashed] (motor.west) -- ++(-0.6,0) |- node[below, pos=0.25, font=\tiny]{heat} (tc.south);
\end{tikzpicture}
\end{document}
```

The dashed path is the part that makes this a loop rather than a limiter: commanded
current heats the motor, which is what the thermocouple eventually reads.

## Control law

This is a **time-based** derate. While the over-temperature condition holds, the
factor decays exponentially toward a floor; once it clears, it climbs back toward 1.0.

$$
f_{k+1} =
\begin{cases}
f_{\min} + (f_k - f_{\min})\, e^{-\Delta t / \tau_d} & \text{over temperature} \\[4pt]
1 - (1 - f_k)\, e^{-\Delta t / \tau_r} & \text{recovered}
\end{cases}
$$

The commanded torque is $\tau_{\text{cmd}} = f \cdot \tau_{\text{request}}$.

Written incrementally like this, the law never stores an absolute timestamp, stays
numerically stable for any $\Delta t$, and cannot leave $[0, 1]$.

### Parameters

| Constant | Value | Meaning |
|---|---|---|
| `DERATE_T_TRIP_C` | 48.89 °C (120 °F) | derate engages above this |
| `DERATE_T_CLEAR_C` | 46.11 °C (115 °F) | must fall below this to recover |
| `DERATE_TAU_DERATE_S` | 10 s | $\tau_d$, decay time constant |
| `DERATE_TAU_RECOVER_S` | 30 s | $\tau_r$, recovery time constant |
| `DERATE_F_MIN` | 0.30 | floor — never command below 30 % |

Defined at [derate.h:24](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/include/derate.h:24).

### What it does in practice

Starting at full command and going over temperature:

| Time over trip | Factor |
|---|---|
| 0 s | 1.000 |
| 10 s ($1\tau_d$) | 0.558 |
| 20 s | 0.395 |
| 30 s | 0.335 |
| 50 s ($5\tau_d$) | 0.305 |

Recovery from 0.558 once back under 115 °F: 0.837 after 30 s, 0.940 after 60 s.

## Implementation

| Piece             | Where                                                                                                            |
| ----------------- | ---------------------------------------------------------------------------------------------------------------- |
| The law           | [derate.c:20](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/derate.c:20) |
| Hysteresis branch | [derate.c:38](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/derate.c:38) |
| Decay step        | [derate.c:47](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/derate.c:47) |
| Recovery step     | [derate.c:54](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/derate.c:54) |
| Sensor read       | [main.cpp:49](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/main.cpp:49) |
| CAN output (stub) | [main.cpp:92](vscode://file/mnt/lab-files/vault-mm/10-Projects/Spartan-Velocity/Firmware/vcu-fw/src/main.cpp:92) |

The MAX31855 converts in 70–100 ms, so the control update runs at 10 Hz paced by the
sensor, while the CAN command goes out at 100 Hz holding the last factor — most
inverters fault if the command message stops arriving.

`derate_update()` takes no clock and touches no hardware, which is why all seven
behaviours below are testable on a laptop with `pio test -e native`:

1. stays at 1.0 below the trip point
2. one $\tau_d$ over temperature gives 0.558
3. five $\tau_d$ settles on the floor
4. hysteresis holds between 115 °F and 120 °F
5. recovery closes 63 % of the gap in one $\tau_r$
6. a sensor fault commands zero and latches
7. $\Delta t$ of zero, negative, or absurdly large all stay in bounds

## Open questions

Ordered roughly by how much they matter.

- [ ] **What is the thermocouple actually attached to, and what is that part rated for?**
      Everything below depends on this answer and it is not written down anywhere.
      48.89 °C is far below the usual limits: class-H motor windings run to 180 °C,
      inverter baseplates derate from roughly 80–100 °C, coolant from 60–70 °C.
- [ ] **120 °F may be below ambient.** A dark component sitting in the sun can exceed
      48.9 °C with the bike switched off. If that happens the VCU powers on already
      derating and the clear condition at 115 °F may never be met. At minimum the
      startup path needs to handle "already over temperature at boot".
- [ ] **The sensor is wrong for this threshold.** MAX31855 is about ±2 °C typical, and
      class-2 K-type wire adds ±2.5 °C — roughly ±4.5 °C total, at a threshold where the
      whole interesting band is maybe 20 °C wide. A 10 kΩ NTC or PT1000 would be about
      ±0.5 °C here and cheaper. K-type earns its place above ~200 °C. If 120 °F is a
      placeholder for a real 150 °C+ limit, this objection disappears — but then the
      constant needs changing.
- [ ] **A thermocouple that falls off but stays wired reads ambient and sets no fault
      bit.** Protection is silently gone with no indication. The MAX31855 detects open
      circuit and shorts, not detachment. Worth a plausibility check: if the reading
      never moves while the motor is clearly working, treat it as suspect.
- [ ] **Should a sensor fault command zero, or a limp value?** Right now it commands
      zero and latches. That protects the hardware, but losing propulsion at speed on a
      motorcycle is its own hazard. This is a safety decision, not a coding one.
- [ ] **Does the derate apply to regen?** Multiplying a negative torque command by the
      factor reduces regenerative braking, which changes brake feel. If the heat source
      is the inverter or pack, derating regen is right; if it is the motor, maybe not.
      Pick one deliberately and add a test for it.
- [ ] **A time-based law ignores severity.** 121 °F and 200 °F derate at exactly the same
      rate, because only elapsed time drives the decay — this law bounds nothing. Scaling
      the time constant by overshoot keeps the time-based feel and actually bounds
      temperature:
      `tau_eff = TAU_DERATE_S / (1 + k * max(0, T - T_TRIP))`.
      One line, and worth doing once the threshold question above is settled.
- [ ] **Is a 30 % floor right?** Is limping better than cutting out, or is there a
      temperature where torque should go to zero outright?
- [ ] **Interaction with the Orion BMS 2 derate.** Two systems independently reducing
      current can compound. Which one wins, and does the VCU need to know?
- [ ] **CAN command message is unverified.** See [[VCU]].

## Log

- **2026-10-16** — Law specified as time-based, implemented, seven host tests passing.
  CAN output deliberately left as a stub.
