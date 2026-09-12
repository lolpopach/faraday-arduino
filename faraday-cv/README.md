# faraday-cv

**Colour-segmentation video analysis** for the Faraday's law pendulum
experiment.

It tracks the pendulum magnet by colour to recover its **position and
instantaneous speed**, synchronises a separately uploaded **Arduino voltage
log** using an LED timing marker, and draws the paper's figures on one shared
time axis.

**The tracking happens inside your browser** (the colour segmentation is
reimplemented in JavaScript). The server receives only the result — a few
coordinates per frame — plus the voltage log, and computes the physics and the
figures. The video itself is never uploaded anywhere. That is what lets this
server run on a small free tier (Render, Fly.io) and be shared by a class. If
you only want it on your own machine, run it locally instead.

> Companion paper: _Beyond "Faster Magnet, More Voltage": A Quantitative
> Faraday's Law Experiment Using Computer Vision_ — putting the coil **near a
> turning point** rather than at the lowest point separates the instant of
> maximum speed from the instant of maximum induced voltage, so students can
> watch the "faster means bigger" rule break in their own data.

---

## What comes out

| Output                        | Contents                                                                               |
| ----------------------------- | -------------------------------------------------------------------------------------- |
| `fig2_motion_and_voltage.png` | Coil distance / speed / induced voltage on one time axis (the paper's Fig. 2)          |
| `fig3_emf_over_velocity.png`  | ℰ beside ℰ/\|**v**\| — which by Eq. (3) tracks −N dΦ/ds (the paper's Fig. 3)           |
| `diagnostics.png`             | Tracking quality: centroid, blob area, LED trace and its threshold                     |
| `synced.csv`                  | The synchronised table: `t, voltage, speed, distance, emf_over_v` — reanalyse anywhere |
| `motion.csv`, `track.csv`     | Motion in SI units / raw per-frame tracking                                            |
| `summary.json`                | Both peak instants and values, detection rate, and every setting used                  |

A record longer than about four seconds also gets `*_detail.png` copies of
Fig. 2 and Fig. 3, zoomed to three periods around the strongest |ℰ| peak —
ten swings drawn at figure width are a picket fence.

---

## Using it on the web

If someone has already deployed it, just open the address in a browser.
Nothing to install, and the video is not sent to their server either — it is
processed entirely in your own browser.

To deploy your own, see [Deployment](#deployment).

## Running the web UI locally

Use a virtual environment. (It also avoids the `externally-managed-environment`
error that the system Python on macOS gives for `pip install`.)

```bash
cd faraday-cv
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
python3 -m faradaycv serve
```

- From the second run on, `source .venv/bin/activate` and then the last line.
- On Windows use `py` instead of `python3`, and `.venv\Scripts\activate`.
- You do not need a `pip` command on macOS or Linux — always use `python3 -m pip`.
- Run these **from inside the `faraday-cv` folder**. `No module named faradaycv`
  almost always means you are somewhere else.

Open the address printed in the terminal (`http://127.0.0.1:8000` by default);
`Ctrl+C` stops it. If the port is taken, pass `--port 8080`.

In the browser, in order:

1. **Choose a video** — mp4/mov/webm and so on. This is **not an upload**: the
   file is played and analysed on this machine, so there is no size limit and
   the video never leaves the browser.
   - **Analysis range** — scrub to the moment you want and press "Use current"
     to set the start and end. Trimming the setup at either end (a hand
     entering the frame, for instance) makes tracking faster and stops it
     jumping to the wrong object. Equivalent to `--start-frame` /
     `--end-frame` on the CLI.
     **The LED is always read from the very beginning of the clip**, whatever
     the range is — the flash usually happens right after recording starts,
     which is exactly the part you want to trim, and it is the reference that
     ties the video to the voltage log. So the synchronisation survives a
     range that begins after it.
2. **Click the magnet** → an HSV range is picked for you; refine with the
   sliders. (The green overlay is "pixels currently counted as the magnet".)
3. **Click the coil centre**, **drag a box over the LED**, and check the
   camera's **fps**.
   - **Length calibration** — in mode ③, drag across something whose real size
     you know (a ruler, the coil). Under the canvas you get
     `256.0 px is really [   ] mm`. **You have to type the real length** for a
     scale to exist. Nothing is applied until you do, so a stray drag cannot
     silently set a wrong scale.
   - The readout also reports the **whole frame width** at that scale
     (`whole frame ≈ 90.6 cm wide`). That is the number to sanity-check: a
     mistyped length is the one calibration error with no other symptom, since
     every distance and speed comes out wrong by the same factor and the curves
     keep their shape.
   - Drag again after calibrating and it simply **measures** that length in mm
     (`≈ 150.0 mm at the current scale`). Type a new value to recalibrate.
4. **Choose the Arduino voltage file** — separately from the video.
5. **Track + analyse** → the browser walks the whole clip extracting
   coordinates (with a progress bar) and sends only those to the server, which
   returns the figures, the diagnostics plot and the CSVs.

If segmentation struggles, drag a **search region** in ⑤ around just the strip
the magnet passes through; everything of a similar colour outside it is then
ignored.

## A one-minute try, with no apparatus

Generate a synthetic dataset (video + voltage log + ground truth) and run the
whole CLI flow on it. The first command writes `example-data/pendulum.mp4` and
`voltage.csv`; you can also drop those two straight into the web UI.

```bash
python3 -m faradaycv.synthetic example-data
python3 -m faradaycv analyze example-data/pendulum.mp4 \
    --voltage example-data/voltage.csv \
    --hsv 170,10,120,255,80,255 \
    --led-roi 16,16,34,34 \
    --mm-per-px 2.0833 --coil 397,332 \
    -o example-data/out
```

Output:

```
LED onset       : frame 6 -> t0 = 0.200 s
max speed       : 0.677 m/s at t = 1.724 s
max |emf|       : 20.61 mV at t = 1.534 s
peak separation : -0.190 s (speed at the emf peak: 0.453 m/s)
```

The two peaks are **0.19 s apart**; at the voltage peak the magnet is moving at
67 % of its top speed, and at top speed the voltage is only 7 % of its peak.
That is the paper's result.

> A record covering several swings has several near-identical speed maxima, so
> the "time of maximum speed" used for the comparison is the one **nearest the
> voltage peak** — otherwise it would jump between swings on noise alone.

## Deployment

The server never touches video (numpy/scipy/matplotlib/flask only — no OpenCV,
no ffmpeg), so a very small instance is enough.

**Render** — put this repository on GitHub, then in Render pick "New +" →
"Blueprint" → this repository. Render finds `render.yaml` at the **repository
root** (outside this `faraday-cv/` folder, at `faraday-arduino/render.yaml`),
and the `rootDir` in it points the build at `faraday-cv/`. The free plan is
enough.

> If Render cannot find `render.yaml` and fails looking for a Dockerfile
> ("open Dockerfile: no such file or directory"), you probably created a
> "Web Service" by hand rather than a Blueprint. Set **Root Directory** to
> `faraday-cv`, **Runtime** to `Python 3`, **Build Command** to
> `pip install -r requirements-web.txt`, and **Start Command** to
> `gunicorn --bind 0.0.0.0:$PORT --workers 2 --threads 4 --timeout 60 wsgi:app`.

**Fly.io**:

```bash
cd faraday-cv
fly launch   # finds the Dockerfile and fly.toml and uses them as they are
fly deploy
```

**Docker, anywhere**:

```bash
cd faraday-cv
docker build -t faraday-cv .
docker run -p 8000:8000 faraday-cv
```

All three set `FARADAYCV_LOCAL_MODE=0`, which turns off the server-side video
decoding (a local-only feature — see [Video formats](#video-formats)) and
leaves only the browser-tracking page. The other variables:

| Variable                        | Meaning                                                                | Default       |
| ------------------------------- | ---------------------------------------------------------------------- | ------------- |
| `FARADAYCV_LOCAL_MODE`          | `0` = public mode (server-side video handling disabled)                | `1` (local)   |
| `FARADAYCV_SESSION_TTL_MINUTES` | How long a run's figures and CSVs are kept before a sweep deletes them | `1440` (24 h) |

A small free instance can be shared by a whole class, so keeping
`FARADAYCV_SESSION_TTL_MINUTES` down at around 60 is sensible — `render.yaml`
and `fly.toml` already do.

> Results live on disk under that session id and are read back from there, so
> they survive the server running more than one worker process. They do **not**
> survive a restart: a free instance that spins down loses them, and the page
> then says so and asks you to run the analysis again.

## Command line (local only)

The CLI **decodes video itself with OpenCV** rather than in a browser — a
separate path from the web UI, meant for scripting and batch work.

```bash
python3 -m faradaycv info swing.mp4
python3 -m faradaycv frame swing.mp4 --index 0 --out f.png
python3 -m faradaycv pick swing.mp4 --at 320,180
python3 -m faradaycv track swing.mp4 --hsv 170,10,120,255,80,255 -o track.csv
python3 -m faradaycv analyze swing.mp4 --voltage log.csv --hsv ... -o out/
python3 -m faradaycv serve
```

| Command   | What it does                                             |
| --------- | -------------------------------------------------------- |
| `info`    | fps, frame count, resolution                             |
| `doctor`  | Diagnoses a video that will not open                     |
| `frame`   | Saves one frame as an image (to find colour coordinates) |
| `pick`    | Estimates an HSV range at a given pixel                  |
| `track`   | Per-frame centroids as CSV                               |
| `analyze` | The whole analysis: figures, tables, summary             |
| `serve`   | The web UI                                               |

Main options for `analyze`:

| Option                                                         | Meaning                                                                 |
| -------------------------------------------------------------- | ----------------------------------------------------------------------- |
| `--hsv h_lo,h_hi,s_lo,s_hi,v_lo,v_hi`                          | Colour range (OpenCV HSV, hue 0–179). `h_lo > h_hi` wraps around red    |
| `--roi x,y,w,h`                                                | Restrict the search region                                              |
| `--min-area`, `--blur`, `--open`, `--close`                    | Mask clean-up parameters                                                |
| `--led-roi x,y,w,h`                                            | LED synchronisation box (without it, set `--t0-video` by hand)          |
| `--mm-per-px`, or `--scale-line x0,y0,x1,y1 --scale-length mm` | Length calibration                                                      |
| `--coil x,y`                                                   | Coil centre (needed for the distance curve)                             |
| `--smooth`                                                     | Savitzky–Golay window in frames, used to fit the velocity               |
| `--v-min`                                                      | Speed floor for ℰ/\|**v**\| — see below. Default: 8 % of the peak speed |

### The speed floor used for ℰ/|v|

At a turning point the magnet really does stop, so ℰ/\|**v**\| would run away.
Rather than drop those samples and leave the curve full of holes, the
**denominator** is held at a floor: below `v_min` the plotted value is
ℰ/`v_min`, not ℰ/\|**v**\|.

- Those samples are flagged by the `emf_over_v_floored` column in
  `synced.csv`, so it is always visible which points are which.
- **The speed curve itself is never floored.** Fig. 2 shows the real speed,
  reaching zero at the turning points, because that is what a pendulum does.
- The default floor is 8 % of the peak speed. A physically motivated choice is
  the smallest speed the frame rate can resolve at a turning point,
  π·v_peak/(T·fps) — for a 1.05 s period at 30 fps that is about 10 % of the
  peak. Below that you are not measuring speed, you are measuring the sampling.
- `--v-min 0` turns the floor off entirely: the ratio is then exact wherever
  the magnet moves and left blank where the speed is exactly zero.

### What Fig. 3's shading means

The shaded bands are **not** simply "where the floor was applied". They mark
where ℰ/\|**v**\| is not a measurement at all.

At a turning point ℰ and \|**v**\| both go to zero together, so their ratio
does have a finite limit — but recovering it needs the video clock and the
voltage clock to agree far better than one video frame, and at 30 fps the
turning instant is only known to within half a frame. Shifting the video clock
by that half frame moves ℰ/\|**v**\| there by more than its own value, while
leaving it almost untouched where the magnet is moving:

| Speed at that instant | How much half a frame of sync error moves ℰ/\|**v**\| |
| --------------------- | ----------------------------------------------------- |
| 30–50 cm/s            | 7 %                                                   |
| 20–30 cm/s            | 25 %                                                  |
| 10–20 cm/s            | 58 %                                                  |
| 5–10 cm/s             | 111 %                                                 |

So every sample carries `emf_over_v_uncertainty_Vs_per_m` in `synced.csv`, and
one is shaded — and marked by `emf_over_v_unreliable` — when that uncertainty
exceeds half the value itself. On a typical run this is about 15 % of the
record, all of it around the turning points.

**Do not quote the peak of ℰ/\|**v**\| as a result.** It sits inside the
shaded region and it does not converge: lowering the floor from 6.7 to
0.4 cm/s on one real record moved it from 0.72 to 2.04, a factor of 2.8. Quote
the value at the |ℰ| peak instead, where the magnet is still moving at about
60 % of top speed and the uncertainty is around 13 %. Read Fig. 3 for the
shape and the sign structure, not for the height of the spikes.

## Arduino

Upload `faraday_logger/faraday_logger.ino` from the repository root.

- **ADS1115** 16-bit ADC over I²C (A4/A5), coil across AIN0–AIN1 as a
  **differential input** — so the sign of the induced voltage is preserved.
- Gain `GAIN_FOUR` (±1.024 V, 0.03125 mV per bit), sampled at **100 Hz**. If
  the log clips at ±1024 mV, step down to `GAIN_TWO`.
- **The LED on D7 is the synchronisation marker.** The instant the sketch
  switches it on is t = 0 in the voltage log; the **first video frame in which
  it appears lit** is t = 0 in the video. The LED must therefore be inside the
  camera frame.
- Output is `time_s,voltage_mV` CSV, after typing `start` in the serial
  monitor. To capture it from a PC:

```bash
python3 -m pip install pyserial
python3 tools/serial_logger.py --list
python3 tools/serial_logger.py --port /dev/ttyACM0 --out voltage.csv --seconds 20
```

`--list` prints the available ports; on macOS they are usually
`/dev/tty.usbmodem…`.

Copying the serial monitor's output into a file by hand works just as well, and
a log from a different sketch or logger can be used as it is. Things seen in
real logs and handled:

- `#` comments, **no header at all**, comma/semicolon/tab/space separators.
- Time in ms, µs or s and voltage in mV or V are **detected automatically**
  (`--voltage-unit` forces it). With no header: a value beyond the ADS1115's
  ±6.144 V limit means millivolts, and values landing on multiples of an
  ADS1115 LSB (0.03125 mV at `GAIN_FOUR`) mean millivolts. The verdict is
  always reported in the run log and in the web UI — check it.
- **Unused empty columns** (`t,v,0,0,0,…`) are ignored.
- **Rows whose timestamps go backwards** (a leftover first line from a previous
  run, or a repeated tail value) are **dropped, not re-sorted**: sorting them in
  would stretch the record and invent a gap that never existed. The count of
  dropped rows is reported.

## Video formats

**The web UI** accepts anything the browser can play — current Chrome, Edge and
Safari handle mp4 (H.264/HEVC) and webm (VP9/AV1). A file the browser cannot
open reports an error immediately; re-encoding usually fixes it:
`ffmpeg -i original.mp4 -c:v libx264 -pix_fmt yuv420p -an swing.mp4`.

**The CLI** (local, OpenCV) is more forgiving — mp4, mov, avi, mkv and so on go
in directly, and a codec OpenCV cannot read (iPhone HEVC, for instance) is
**converted automatically** to a constant-frame-rate H.264 copy using the
ffmpeg installed alongside. That conversion exists **only in local mode**, not
in a public deployment: the video never reaches the server, so the server
cannot convert it for you.

If a video will not open, **find out why first**:

```bash
python3 -m faradaycv doctor /path/to/swing.mp4
```

`moov atom is missing` is **not a codec problem — it is an incomplete file**.
MP4 keeps the index needed for playback (`moov`) at the end, so a copy that was
cut short looks the right size but no program can open it. On an iPhone, export
again with **File → Export → Export Unmodified Original**, wait for the iCloud
download to finish, and compare the byte size.

## Setting up the experiment

- Put a **plain coloured marker on the magnet** in a colour that appears nowhere
  else (red, or fluorescent tape). Keep anything of that colour off the bench.
- Mount the camera — a **webcam or a smartphone** — on a tripod,
  **perpendicular to the plane of the swing**. If it moves, every pixel
  coordinate moves with it.
- Keep the lighting steady. If mains flicker is bad, use a camera with a short
  shutter.
- Put the coil **near a turning point, not at the lowest point** — that is the
  whole device for separating speed from position.
- Calibration: film a **ruler or an object of known length** in the frame, drag
  across it in the web UI, and type the real length in mm.
- Set the web UI's tracking fps to the camera's **actual** fps. A phone's frame
  rate wobbles slightly while recording (variable frame rate); the analysis uses
  the time the browser reports for each frame, so that is usually fine, but an
  fps far from the truth can skip or double-count frames.

## When it does not work

| Symptom                                       | What to check                                                                                                                                                                                                                                                             |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Low detection rate (`detected in only …%`)    | Lower the S/V minimums and widen the H range. Try a smaller `--min-area`                                                                                                                                                                                                  |
| Several blobs are found                       | Set a search region (ROI), or narrow the H range                                                                                                                                                                                                                          |
| Tracking jumps to an arm or to skin           | A search region (ROI) around just the strip the magnet passes through is the surest fix. After that, set **Max jump (px/frame)** in "2. Colour range" to ignore detections further away than that and hold the previous position                                          |
| `LED never crossed the on-threshold`          | Check that the LED box really covers the LED; otherwise set `--t0-video` / "manual t₀"                                                                                                                                                                                    |
| `LED region is bright in every frame`         | Recording started after the LED came on. **Start recording first**, then reset the Arduino                                                                                                                                                                                |
| `records do not overlap`                      | The log and the video are from different runs, or t₀ is wrong                                                                                                                                                                                                             |
| ℰ/\|**v**\| spikes at the turning points      | Expected — the speed goes to zero there. Raise `--v-min`                                                                                                                                                                                                                  |
| The speed curve is a sawtooth                 | Raise `--smooth` (a frame count, odd). The velocity is fitted, not differenced, so this should be rare                                                                                                                                                                    |
| The video will not open in the web UI         | If the message says the **index (`moov`) is missing**, it is a truncated file, not a codec problem — re-encoding will not help, you need the original again (large-file transfer apps are a common cause). Any other message is a real codec problem: export as H.264 mp4 |
| Tracking in the web UI takes a long time      | A long or high-resolution clip. A search region (ROI) speeds it up                                                                                                                                                                                                        |
| "Open by path" / server upload is missing     | Deliberately off in public mode (`FARADAYCV_LOCAL_MODE=0`). Use browser tracking                                                                                                                                                                                          |
| `cannot read that video`                      | Run `python3 -m faradaycv doctor FILE` first (a local/CLI feature)                                                                                                                                                                                                        |
| `moov atom is missing` / `Invalid data found` | A truncated file, not a codec. Export or copy the original again                                                                                                                                                                                                          |
| The download fails, or saves a renamed file   | The run expired on the server. Press Run again                                                                                                                                                                                                                            |
| `zsh: command not found: pip`                 | macOS has no `pip` command. Use `python3 -m pip`                                                                                                                                                                                                                          |
| `No module named 'flask'` / `'cv2'`           | The install step was skipped — see **Running the web UI locally**                                                                                                                                                                                                         |
| `No module named faradaycv`                   | You are outside the `faraday-cv` folder                                                                                                                                                                                                                                   |
| `zsh: command not found: #`                   | A trailing comment was copied with the command. Interactive zsh does not treat `#` as a comment                                                                                                                                                                           |
| `externally-managed-environment`              | Installing into the system Python. Make a venv, or add `--user`                                                                                                                                                                                                           |

## Development

```bash
python3 -m pip install -r requirements.txt   # everything, including OpenCV
python3 -m pytest -q
node tests/browser/cv.test.mjs               # browser colour-segmentation unit tests
ruff check faradaycv tests
ruff format --check faradaycv tests
```

The Python tests check against the ground truth of a synthetic dataset. They
really encode and decode a video every run, and require tracking within 1.5 px,
speed within 0.05 m/s, the exact LED frame, the voltage peak within 20 ms — and
the paper's conclusion, that maximum speed and maximum voltage do not coincide.
They also check that the analysis pipeline still runs in an environment with
only `requirements-web.txt` installed, without OpenCV.

The browser side (`static/cv.js`) is covered by pure unit tests that run
straight in Node, and if Playwright is installed
(`pip install playwright && playwright install chromium`),
`tests/browser/test_e2e.py` drives the real page in a headless browser all the
way through to the server's results — it skips quietly if not. The headless
Chromium in this test environment has no H.264 decoder, so the test substitutes
another codec; what that does and does not tell you about tracking accuracy is
written down honestly in `tests/browser/README.md`.

```
faradaycv/
  track.py          Tracking results, independent of where the video came from (no cv2)
  segmentation.py   HSV ranges, mask clean-up, blob choice, click-to-colour (OpenCV, lazy import)
  video.py          OpenCV decoding + tracking (CLI/local only)
  decode.py         Diagnoses unplayable video, converts to H.264 when needed (CLI/local only)
  voltage.py        Arduino CSV parser (units, separators and headers detected)
  analysis.py       Pixels to metres, smoothing, fitted velocity, time sync, ℰ/|v|
  plots.py          The paper's Fig. 2 / Fig. 3 and the diagnostics figure
  pipeline.py       Track -> analysis -> files (no OpenCV needed)
  webapp.py         Web backend: /api/analyze (light, for public deployment) + server-side processing (local only)
  synthetic.py      Synthetic dataset generator (demo + test ground truth, H.264)
  cli.py            Command line
static/
  cv.js             Browser colour segmentation (the JS twin of segmentation.py)
  tracker.js        <video> frame walk + the whole tracking loop
  app.js, style.css, index.html   Web UI
../faraday_logger/        ADS1115 + LED marker sketch (at the repository root)
tools/serial_logger.py    Serial -> CSV
wsgi.py, Dockerfile, fly.toml   Deployment
../render.yaml            Render Blueprint (at the repository root, where Render looks)
```

---

## Authors and licence

Software accompanying _Beyond "Faster Magnet, More Voltage": A Quantitative
Faraday's Law Experiment Using Computer Vision_.

**Ui Chan Kim · Ye Geon Kim · Chan Hee Yang · Yongseok Jeong**

© 2026, released under the [MIT licence](../LICENSE). Free to use, modify and
redistribute, as long as the copyright notice and the licence text travel with
it.

If you publish results obtained with this tool, please cite the paper above.
