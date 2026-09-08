<div align="center">

# ⚡ faraday-arduino

**Voltage logger and LED timing marker for a quantitative Faraday's law experiment**

Sample the emf induced in a coil by a swinging magnet, and stamp the record with an
LED flash so it can be laid on the same time axis as a video of the motion.

[![Platform](https://img.shields.io/badge/platform-Arduino%20UNO-00979D)](https://www.arduino.cc/)
[![ADC](https://img.shields.io/badge/ADC-ADS1115%2016--bit-orange)](https://www.ti.com/product/ADS1115)
[![Sampling](https://img.shields.io/badge/sampling-100%20Hz-blue)]()
[![Companion](https://img.shields.io/badge/video%20analysis-faraday--cv-6f42c1)](https://github.com/lolpopach/general-use/tree/claude/color-segmentation-program-4ex0y8/faraday-cv)

</div>

---

## The question this experiment asks

In most classrooms Faraday's law arrives as a demonstration: push a magnet through a
coil, watch the needle twitch, conclude that **a faster magnet makes a bigger voltage.**

That rule is a shortcut, and this experiment is built to break it.

Faraday's law says the induced emf follows the _rate of change of magnetic flux_:

$$\varepsilon = -N\frac{d\Phi}{dt}$$

Because the flux linked by the coil depends on where the magnet is, the chain rule
splits that into two factors:

$$\varepsilon = -N\frac{d\Phi}{dx}\cdot\frac{dx}{dt} = -N\frac{d\Phi}{dx}\cdot v$$

Speed $v$ is only **half** of it. The other half, $d\Phi/dx$, depends entirely on
_where_ the magnet sits relative to the coil. A fast magnet in a place where the flux
barely changes with position produces a small emf; a slow magnet where the flux changes
steeply produces a large one.

### Why a pendulum, and why the coil goes near the turning point

A pendulum is the cheapest way to **decouple speed from position** — no motion-control
gear required. It is fastest at the bottom of its swing and slowest at the ends.

So the coil is placed **near the turning point**, _not_ at the bottom:

| Where in the swing | Magnet speed | Distance to coil    |
| ------------------ | ------------ | ------------------- |
| Bottom of swing    | **fastest**  | far from the coil   |
| Near turning point | **slowest**  | closest to the coil |

Those two conditions are now in open conflict, and the data has to pick a side.

### The result

**The instant of maximum speed and the instant of maximum voltage do not coincide.**
The largest voltages appear as the magnet approaches and recedes from the coil — while
it is moving comparatively slowly. Students meet a direct contradiction of their own
prediction, using their own measurements.

For an introductory college course, rearranging the equation above gives a quantity
students can compute straight from the two things they measured:

$$\frac{\varepsilon}{v} = -N\frac{d\Phi}{dx}$$

Plotting $\varepsilon(t)$ against $\varepsilon(t)/v(t)$ reorganises the same data so the
spatial variation of the flux becomes visible, rather than staying an abstraction.

---

## What is in this repository

| Path                                | What it is                                                          |
| ----------------------------------- | ------------------------------------------------------------------- |
| `faraday_logger/faraday_logger.ino` | The sketch: reads the coil voltage, drives the sync LED, prints CSV |
| `README.md`                         | This document                                                       |

The video half of the experiment lives in a separate project,
[**faraday-cv**](https://github.com/lolpopach/general-use/tree/claude/color-segmentation-program-4ex0y8/faraday-cv) —
a no-code colour-segmentation tool that tracks the magnet frame by frame and merges
its track with the CSV this sketch produces.

---

## Hardware

### Parts

| Part                     | Notes                                                                                    |
| ------------------------ | ---------------------------------------------------------------------------------------- |
| Arduino UNO              | Any 5 V AVR board with I²C should work                                                   |
| ADS1115 breakout         | 16-bit ADC. The coil emf is millivolt-scale — the UNO's own 10-bit ADC is far too coarse |
| Coil                     | The paper used **90 mm diameter, 7000 turns**                                            |
| Neodymium magnet         | Hung as a pendulum bob                                                                   |
| LED + 220–330 Ω resistor | The timing marker. **Must be visible in the camera frame**                               |
| Webcam                   | ~30 fps, mounted perpendicular to the plane of the swing                                 |

### Wiring

**ADS1115 → Arduino UNO**

| ADS1115 | Arduino UNO | Note                                          |
| ------- | ----------- | --------------------------------------------- |
| `VDD`   | `5V`        |                                               |
| `GND`   | `GND`       |                                               |
| `SCL`   | `A5`        | I²C clock                                     |
| `SDA`   | `A4`        | I²C data                                      |
| `ADDR`  | `GND`       | Sets address `0x48`, which the sketch expects |
| `A0`    | —           | Coil lead 1                                   |
| `A1`    | —           | Coil lead 2                                   |

The coil is read **differentially across A0–A1**, so neither coil lead is tied to
ground. That rejects common-mode noise, which matters at these signal levels.

**Sync LED → Arduino UNO**

| LED                 | Arduino UNO                       |
| ------------------- | --------------------------------- |
| Anode (long leg)    | `D7` through a 220–330 Ω resistor |
| Cathode (short leg) | `GND`                             |

```
        ┌──────────────┐
 coil ──┤A0        VDD ├── 5V
        │   ADS1115    │
 coil ──┤A1        GND ├── GND
        │   (0x48)     │
        │  SCL     SDA │
        └───┬───────┬──┘
            │       │
           A5      A4          D7 ──[220Ω]──▶|── GND
                                          sync LED
                                    (must be in the camera's view)
```

---

## Setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ADC library: **Tools → Manage Libraries…**, search
   `Adafruit ADS1X15`, install it (accept the Adafruit BusIO dependency prompt).
3. Open `faraday_logger/faraday_logger.ino`, select your board and port, and upload.
4. Open **Tools → Serial Monitor**, set the baud rate to **115200**, and set the line
   ending to **Newline**.

You should see:

```
READY
TYPE start THEN PRESS ENTER
TYPE stop THEN PRESS ENTER
CSV WILL START AFTER start COMMAND
```

> [!NOTE]
> If the LED instead blinks rapidly and forever, the sketch printed
> `ERROR,ADS1115_NOT_FOUND` — the ADC is not answering on `0x48`. See
> [Troubleshooting](#troubleshooting).

---

## Running an experiment

The single most important thing about the procedure is the **order** of two steps.

> [!WARNING]
> **Start the video recording BEFORE you type `start`.**
>
> The LED marks $t=0$ by _turning on_. If the recording begins after the LED is
> already lit, there is no dark-to-lit transition in the video, and the two records
> can no longer be aligned. The analysis tool will tell you the LED "was already lit
> when recording started" — but by then the run is wasted.

### Step by step

1. **Position the coil near the turning point** of the swing, not at the bottom.
   This is the entire point of the experimental design.
2. **Frame the camera** perpendicular to the plane of the swing, with **both the
   magnet's full path and the sync LED inside the frame.**
3. ▶️ **Start recording.** Let it run for a second or two.
4. Pull the magnet aside and **release it.** Let it swing freely.
5. ⌨️ In the Serial Monitor, type **`start`** and press Enter.
   - The LED lights for 0.5 s — this is $t=0$ for the voltage record.
   - CSV rows begin streaming immediately.
6. Let the pendulum swing through the region of interest.
7. Type **`stop`** and press Enter, then **stop the recording.**
8. **Save the CSV**: select all the serial output, copy it into a text file, and save
   it as e.g. `voltage.csv`. Keep only the header line and the numeric rows.

### The shared time origin

```
  voltage record   ──────────────●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━▶
                                 │  t = 0 s   (LED switched on)
                                 │
  video            ──────▶ ▶ ▶ ▶ ●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━▶
                    already      │  first frame in which the LED
                    recording    │  appears lit  →  video t = 0
```

Both records now share an origin, so position $x(t)$, speed $v(t)$ and voltage
$\varepsilon(t)$ can be read off one axis.

---

## Output format

After `start`, the sketch prints a CSV header followed by one row per sample:

```csv
time_s,voltage_mV
0.000123,0.031250
0.010119,-0.062500
0.020117,0.093750
```

| Column       | Meaning                                                                                                                                      |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------- |
| `time_s`     | Seconds since `start` — i.e. since the LED lit. Taken from the moment the sample was actually read, so it stays honest if a sample runs late |
| `voltage_mV` | Differential coil voltage in millivolts                                                                                                      |

Lines that are **not** data (`READY`, `STOPPED`, `UNKNOWN_COMMAND,…`) are status
messages; delete them before analysis. The companion tool tolerates a stray header or
blank line, but it will not guess at prose.

---

## Settings you may want to change

All at the top of the sketch:

| Constant             | Default          | Change it when                                                                                                                                      |
| -------------------- | ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `SAMPLE_INTERVAL_US` | `10000` (100 Hz) | You want finer time resolution. 100 Hz is comfortably faster than a 30 fps camera; going much below ~5000 µs risks the serial output not keeping up |
| `LED_DURATION_US`    | `500000` (0.5 s) | The camera is slow — the flash must span several frames. At 30 fps, 0.5 s is ~15 frames                                                             |
| `SYNC_LED_PIN`       | `7`              | You wired the LED elsewhere                                                                                                                         |

And in `setup()`:

| Call                                   | Default                  | Change it when                                                                                                     |
| -------------------------------------- | ------------------------ | ------------------------------------------------------------------------------------------------------------------ |
| `ads.setGain(GAIN_FOUR)`               | ±1.024 V, 0.03125 mV/bit | Your peaks clip (use a **lower** gain, e.g. `GAIN_TWO` for ±2.048 V) or are tiny (use `GAIN_EIGHT`/`GAIN_SIXTEEN`) |
| `ads.setDataRate(RATE_ADS1115_860SPS)` | 860 SPS                  | Rarely. A slower rate is quieter but each conversion takes longer, which caps your sampling rate                   |

> [!TIP]
> Check your first run for clipping: if `voltage_mV` flattens at about ±1024, the
> signal is hitting the rail of `GAIN_FOUR`. Drop to `GAIN_TWO` and repeat.

---

## Pairing it with the video

1. Record the video and the CSV as above.
2. Open [**faraday-cv**](https://github.com/lolpopach/general-use/tree/claude/color-segmentation-program-4ex0y8/faraday-cv).
3. Load the video, click the magnet to pick its colour, mark the coil position, drag a
   box over the LED, and load this CSV as the voltage file.
4. Run. The tool finds the frame in which the LED lights, uses it as video $t=0$,
   aligns the two records, and produces the figures — position and speed against
   voltage on one axis, and $\varepsilon$ against $\varepsilon/v$.

Tracking happens inside the browser; the video is never uploaded.

---

## Using it in the classroom

The experiment is at its best run as **Predict → Observe → Explain**, with the data
arriving _before_ the theory.

**🔮 Predict.** Demonstrate the swing and ask:

> _"At which point in the swing will the induced voltage be largest?"_

Most students will say: at the bottom, where the magnet moves fastest. Record the
class's predictions before continuing — the disagreement later is the whole lesson.

**🔬 Observe.** Students record their own run and analyse it, then read the peak speed
and the peak voltage off a common time axis. The two peaks are visibly apart.

**💡 Explain.** Ask:

> _"Why was the voltage not largest when the magnet was moving fastest?"_
> _"If speed alone cannot explain this, what else must matter?"_

- **High school** — keep it qualitative. The flux through the coil, and how quickly it
  changes, depend on _where_ the magnet is. Interpret with $\varepsilon = -N\,d\Phi/dt$.
- **Introductory college** — go quantitative with
  $\varepsilon = -N(d\Phi/dx)v$, and use the $\varepsilon/v$ plot to bring the spatial
  variation of the flux into the discussion.

---

## Troubleshooting

| Symptom                                                        | Cause and fix                                                                                                                                             |
| -------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| LED blinks fast forever; `ERROR,ADS1115_NOT_FOUND`             | The ADC is not answering at `0x48`. Check `SDA`→`A4`, `SCL`→`A5`, power, and that `ADDR` is tied to `GND`. An I²C scanner sketch will confirm the address |
| Serial Monitor shows garbage characters                        | Baud rate mismatch — set the monitor to **115200**                                                                                                        |
| Typing `start` does nothing                                    | Set the Serial Monitor's line ending to **Newline**. The sketch reads up to `\n`                                                                          |
| Voltage flat at ±1024 mV                                       | Clipping at the `GAIN_FOUR` range. Use `GAIN_TWO`                                                                                                         |
| Voltage is pure noise around zero                              | Coil not actually across `A0`/`A1`, or the magnet is passing too far from the coil                                                                        |
| Analysis says the LED "was already lit when recording started" | The recording began after `start`. Re-run, recording first                                                                                                |
| Analysis says the LED "never crossed the on-threshold"         | The LED is out of frame, or the marked LED box does not cover it. Check that the LED is visible in the video                                              |

---

## Notes and limits

- **Run length.** `micros()` wraps after about 71 minutes. Elapsed-time arithmetic is
  unsigned, so a single run stays correct through one wrap, but there is no reason to
  push it — a run is seconds long.
- **Serial as the recorder.** The CSV comes out over the serial port and is captured by
  whatever is watching it. There is no SD card and no buffering: if the Serial Monitor
  is closed mid-run, those rows are gone.
- **`readStringUntil` can block.** If a byte arrives without a newline, the command
  reader waits up to the serial timeout (1 s by default) for the rest of the line, and
  sampling pauses. In practice nobody types during a run, but avoid sending stray
  characters mid-measurement.
- **Sampling rate is nominal.** The schedule advances in fixed 10 ms steps, but each row
  is stamped with the time the sample was _actually_ taken, so a late sample is reported
  late rather than mislabelled.

---

## Reference

This code accompanies the experiment described in
_"Beyond 'Faster Magnet, More Voltage': A Quantitative Faraday's Law Experiment Using
Computer Vision."_

<div align="center">
<sub>Companion project: <a href="https://github.com/lolpopach/general-use/tree/claude/color-segmentation-program-4ex0y8/faraday-cv">faraday-cv</a> — no-code colour-segmentation video analysis</sub>
</div>
