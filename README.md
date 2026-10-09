# 3D Renderer (C)

A small wireframe 3D renderer written in C. It loads Wavefront `.obj` models,
draws their edges with perspective projection and Cohen–Sutherland clipping,
and lets you tumble, pan and zoom the model interactively.

The camera automatically frames whatever model you load, so both a tiny unit
cube and a large scanned mesh appear at a comfortable size right away.

## Demo

A short screen recording of the renderer in action:

https://github.com/user-attachments/assets/9be68f0c-375c-4146-9f38-ecaa9a4f0b97

## Features

- Wavefront `.obj` loading (vertices and polygon edges, with duplicate-edge removal).
- Perspective projection with an adjustable near/far clip range.
- Cohen–Sutherland line clipping against the view frustum.
- Automatic camera framing based on the model's bounding sphere.
- Bounding-box centering so asymmetric models are framed symmetrically.
- Real-time on-screen HUD: FPS, model name, vertex/edge counts, camera distance, position and rotation.
- Live window recording to H.264 MP4 via an ffmpeg pipe, at a steady frame rate.
- Optional OpenCL port that offloads the per-vertex transform/projection to the GPU (falls back to CPU).

## Requirements

- A C compiler (`gcc` or `clang`) and `make`.
- [SDL3](https://github.com/libsdl-org/SDL) and SDL3_ttf development packages.
- `ffmpeg` in `PATH` (only needed for `--export`).

On Arch Linux:

```sh
sudo pacman -S base-devel sdl3 sdl3_ttf
```

On Debian/Ubuntu (where available):

```sh
sudo apt install build-essential libsdl3-dev libsdl3-ttf-dev
```

The FPS overlay loads `/usr/share/fonts/noto/NotoSans-Bold.ttf`; adjust the path
in `main.c` if your system stores fonts elsewhere.

## Build

From the repository root:

```sh
make
```

This produces the `renderer` binary.

To build the OpenCL variant:

```sh
cd opencl
make
```

The OpenCL build additionally needs an OpenCL runtime/headers (e.g.
`opencl-headers`, `ocl-icd`). It must be run from the `opencl/` directory so it
can find `pipeline.cl`.

## Usage

```sh
./renderer path/to/model.obj
```

For example:

```sh
./renderer models/cube.obj
./renderer models/13463_Australian_Cattle_Dog_v3.obj
./renderer models/xyzrgb_dragon.obj
```

## Recording video

The renderer can record the live window to an H.264 MP4 through an `ffmpeg`
pipe. It captures exactly what is on screen — wireframe and HUD — while you
interact with the model normally.

```sh
./renderer models/cube.obj --record out.mp4
```

Recording continues until you close the window (or hit the `--frames` limit):

```sh
# Stop automatically after 10 seconds at 60 fps
./renderer models/cube.obj --record out.mp4 --frames 600
```

| Option | Description | Default |
| --- | --- | --- |
| `--record <file.mp4>` | Record the live window to this file | off |
| `--fps <F>` | Recording frame rate | `60` |
| `--frames <N>` | Stop after N recorded frames | until window closed |

The capture runs in real time: every rendered frame is grabbed with
`SDL_RenderReadPixels`, converted to RGB24 and streamed to ffmpeg. To keep a
steady `fps` that matches the real-world duration of your session, frames are
duplicated when rendering falls behind and extra renders are dropped when it
runs ahead — so the output is always `fps`, regardless of the actual render
speed.

`ffmpeg` must be installed and on your `PATH`. To run without a visible window
(for example on a server), use the SDL offscreen driver:

```sh
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software \
  ./renderer models/cube.obj --record out.mp4 --frames 300
```

## Controls

| Key / Input | Action |
| --- | --- |
| `W` / `S` | Move model up / down (Y axis) |
| `A` / `D` | Move model left / right (X axis) |
| `Q` / `E` | Move model along Z |
| `I` / `K` | Rotate around X axis |
| `J` / `L` | Rotate around Y axis |
| `U` / `O` | Rotate around Z axis |
| Mouse wheel | Zoom in / out |
| Window close | Quit |

## How the camera framing works

Each frame the vertices are rotated and translated by the user's input, then
moved into camera space by a single translation, projected, clipped and drawn.

Because the projection uses a fixed `15°` vertical half field of view, the
visible half-height at a given depth `d` is `d * tan(15°)`. A hard-coded camera
distance therefore only frames one particular model scale correctly.

`fit_camera()` instead computes the bounding-sphere radius `R` of the loaded,
centered mesh and places the camera at:

```
distance = R / tan(15°) * 1.2
```

so the object fills roughly 83% of the view with a small gap behind it. The
near/far planes are derived from `distance ± R` so the whole model stays inside
the frustum regardless of how large or small it is.

## Project structure

```
main.c               Application loop, rendering, FPS overlay
transformations.c/h  Rotations, translation, projection, centering, camera fit
clip.c/h             Cohen–Sutherland line clipping
obj_loader.c/h       Wavefront .obj parser and mesh storage
events.c/h           Keyboard/mouse input handling
Makefile             CPU build
models/              Sample .obj models
opencl/              Optional OpenCL port (own copies of the sources)
```

## License

MIT — see [LICENSE](LICENSE).