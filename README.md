# ⚡ faraday-arduino

Both halves of the Faraday's law pendulum experiment, in one place.

|                                                                      |                                                                                                                                        |
| -------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| **Voltage** — [`faraday_logger/`](faraday_logger/faraday_logger.ino) | Arduino sketch. Samples the coil at **100 Hz** and flashes an **LED at t = 0**.                                                        |
| **Video** — [`faraday-cv/`](faraday-cv/)                             | No-code colour-segmentation analysis tool. Tracks the magnet in the browser, syncs to the log off that LED, draws the paper's figures. |

The LED is what joins them: the sketch lights it as logging starts, and the
analysis takes t = 0 to be the first video frame in which it appears lit.

---

## Wiring

**ADS1115 → Arduino UNO**

| ADS1115 | UNO   |                                          |
| ------- | ----- | ---------------------------------------- |
| `VDD`   | `5V`  |                                          |
| `GND`   | `GND` |                                          |
| `SCL`   | `A5`  |                                          |
| `SDA`   | `A4`  |                                          |
| `ADDR`  | `GND` | → address `0x48`                         |
| `A0`    | —     | coil lead 1, **and one leg of the 1 kΩ** |
| `A1`    | —     | coil lead 2, **and the other leg**       |

The **1 kΩ sits across the coil, not in series with it**. On the breadboard
each leg goes into its own row, and that row also takes the coil lead and the
wire to the ADC — three things in one row, which is what puts them on the same
node. It loads the coil so it cannot ring, and gives the ADS1115's differential
inputs a DC path instead of leaving them floating.

**LED** → `D7` through a 220–330 Ω resistor → `GND`.
It must be **inside the camera frame**.

```
        ┌──────────────┐
 5V ────┤VDD           │
GND ────┤GND           │            A0 ──┬──────────┬
 A5 ────┤SCL  ADS1115  │                 │          │
 A4 ────┤SDA   (0x48)  │              [ 1 kΩ ]   ( coil )
GND ────┤ADDR          │                 │          │
        │    A0    A1  ├──────▶     A1 ──┴──────────┴
        └──────────────┘

                    D7 ──[220 Ω]──▶|── GND
                                 sync LED
```

---

## Setup

1. Arduino IDE → **Tools → Manage Libraries…** → install **`Adafruit ADS1X15`**.
2. Upload `faraday_logger/faraday_logger.ino`.
3. **Serial Monitor**: baud **115200**, line ending **Newline**.

---

## Running

> [!WARNING]
> **Start recording BEFORE typing `start`.**
> The LED marks t = 0 by _turning on_. If it is already lit when recording
> begins, there is no dark→lit edge to sync on and the run is wasted.

1. Put the coil **near the turning point** of the swing (not the bottom).
2. Aim the camera at the swing — magnet **and LED** both in frame.
3. ▶️ **Start recording.**
4. Release the magnet.
5. Type **`start`** → LED lights 0.5 s, CSV starts streaming.
6. Type **`stop`**, then stop recording.
7. Copy the serial output into a `.csv` file (keep the header + number rows only).

---

## Output

```csv
time_s,voltage_mV
0.000123,0.031250
0.010119,-0.062500
0.020117,0.093750
```

`time_s` counts from the LED flash. Delete any `READY` / `STOPPED` lines.

---

## Settings

| In the sketch        | Default                | Change when                                    |
| -------------------- | ---------------------- | ---------------------------------------------- |
| `SAMPLE_INTERVAL_US` | `10000` (100 Hz)       | you want finer time resolution                 |
| `LED_DURATION_US`    | `500000` (0.5 s)       | slow camera — flash must span several frames   |
| `ads.setGain(...)`   | `GAIN_FOUR` (±1.024 V) | signal clips flat at ±1024 mV → use `GAIN_TWO` |

---

<details>
<summary><b>📖 The physics — why this experiment works</b></summary>

<br>

Classrooms usually reduce Faraday's law to: _a faster magnet makes a bigger
voltage._ This experiment is built to break that shortcut.

$$\varepsilon = -N\frac{d\Phi}{dt} = -N\frac{d\Phi}{ds}\,|\mathbf v|$$

Here $s$ is arc length along the path the magnet actually travels, so
$d\Phi/ds$ is the flux gradient taken along the **instantaneous direction of
motion**, and $|\mathbf v|$ is the speed. Speed is only **half** of it. The
other half, $d\Phi/ds$, depends on _where_ the magnet is relative to the coil.

A pendulum separates the two for free — fastest at the bottom, slowest at the
ends. So the coil goes **near the turning point**:

| Where in the swing | Speed       | Distance to coil |
| ------------------ | ----------- | ---------------- |
| Bottom             | **fastest** | far              |
| Near turning point | **slowest** | closest          |

**Result: the peak speed and the peak voltage do not coincide.** The largest
voltages appear as the magnet approaches and recedes from the coil, while moving
comparatively slowly — contradicting the "faster is always bigger" rule.

For college level, rearranging gives a quantity computable straight from the
measurements:

$$\frac{\varepsilon}{|\mathbf v|} = -N\frac{d\Phi}{ds}$$

Plotting $\varepsilon$ against $\varepsilon/|\mathbf v|$ divides the speed back
out and leaves the spatial variation of the flux visible on its own.

_Apparatus in the paper: coil 90 mm diameter, 7000 turns; ADS1115 + Arduino UNO;
a webcam or smartphone at ~30 fps, perpendicular to the plane of the swing._

</details>

<details>
<summary><b>🎓 Classroom use (Predict → Observe → Explain)</b></summary>

<br>

**Predict.** Demonstrate the swing, then ask:

> _"At which point will the induced voltage be largest?"_

Most students say the bottom, where it moves fastest. **Record the predictions
before continuing** — the disagreement later is the whole lesson.

**Observe.** Students analyse their own video and read the peak speed and peak
voltage off a common time axis. The two peaks are visibly apart.

**Explain.** Ask:

> _"Why was the voltage not largest when the magnet was moving fastest?"_
> _"If speed alone cannot explain it, what else must matter?"_

- **High school** — keep it qualitative: flux depends on _where_ the magnet is.
- **College** — go quantitative with $\varepsilon = -N(d\Phi/ds)|\mathbf v|$
  and the $\varepsilon/|\mathbf v|$ plot.

</details>

<details>
<summary><b>🔧 Troubleshooting</b></summary>

<br>

| Symptom                                             | Fix                                                                                               |
| --------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| LED blinks fast forever (`ERROR,ADS1115_NOT_FOUND`) | ADC not answering at `0x48` — check `SDA`→`A4`, `SCL`→`A5`, power, `ADDR`→`GND`                   |
| Garbage characters in Serial Monitor                | Set baud to **115200**                                                                            |
| Typing `start` does nothing                         | Set line ending to **Newline**                                                                    |
| Voltage flat at ±1024 mV                            | Clipping — use `GAIN_TWO`                                                                         |
| Only noise around zero                              | Coil not across `A0`/`A1`, or magnet passing too far from the coil                                |
| Signal much smaller than expected                   | The 1 kΩ is in series with the coil instead of across it — both legs share a row with a coil lead |
| "LED was already lit when recording started"        | Recording began after `start` — re-run, recording first                                           |
| "LED never crossed the on-threshold"                | LED out of frame, or the marked LED box misses it                                                 |

</details>

<details>
<summary><b>⚠️ Known limits</b></summary>

<br>

- **`micros()` wraps after ~71 minutes.** Fine for runs of seconds; elapsed-time
  arithmetic is unsigned so a single wrap is handled.
- **No SD card.** The CSV exists only in whatever is watching the serial port —
  close the Serial Monitor mid-run and those rows are gone.
- **`readStringUntil` can block.** A byte without a newline stalls sampling for up
  to the 1 s serial timeout. Don't send stray characters mid-run.
- **Sampling rate is nominal**, but each row is stamped with the time the sample
  was _actually_ taken, so a late sample is reported late, not mislabelled.

</details>

---

<sub>Accompanies _"Beyond 'Faster Magnet, More Voltage': A Quantitative Faraday's Law Experiment Using Computer Vision."_</sub>

<sub>© 2026 Ui Chan Kim, Ye Geon Kim, Chan Hee Yang, Yongseok Jeong · released under the [MIT licence](LICENSE) — free to use and modify, as long as the copyright notice travels with it.</sub>
