# Building on a modern Linux (Ubuntu 26.04 test)

The Makefile predates modern distros. Here's what it takes to get a working
`m` binary today, and why each step is needed.

## 1. Install matching FLTK + fluid + SDL1.2

```
apt-get install -y libfltk1.4-dev fluid libsdl1.2-dev build-essential
```

**Why the version matters:** `fluid` and `fltk-config` must be the *same*
FLTK version. Installing `libfltk1.3-dev` pulls in a `fluid` binary from a
different package (1.4.x) via its `Recommends`, so `fluid` generates code
using 1.4-only headers (e.g. `Fl_Anim_GIF_Image.H`) while `fltk-config`
still points at 1.3 — compile fails with "No such file or directory" for a
header that simply doesn't exist in 1.3. Fix: install `libfltk1.4-dev` too
so both tools agree on 1.4. Check with `fltk-config --version` and
`fluid --version` — they must match.

## 2. `help.py` needs Python 3

`src/help.py` had a `#!/usr/bin/python` shebang (doesn't exist on modern
systems, only `python3`) and Python 2 `print` statements. Fixed in this
repo: shebang → `#!/usr/bin/python3`, `print "..."` → `print("...")`.
(If `make` fails with `./help.py: not found`, this is why.)

Also: if `make` fails on this step and you fix it afterward, delete the
empty `Help.h` the failed run left behind before re-running `make` —
`make` won't regenerate a target that already exists and is newer than
its dependency.

## 3. Don't use the `-static` Makefile targets

The `m` and `m.exe` targets in the Makefile link with `-static` and
`sdl-config --static-libs`. Neither works on a modern system:

- `sdl-config --static-libs` doesn't exist in the SDL1.2-compat package
  shipped today (it wraps SDL2); only `--cflags`/`--libs` are supported.
- Fully static linking needs static `.a` files for everything FLTK 1.4
  pulls in (cairo, pango, harfbuzz, Xft, wayland-client, xkbcommon...).
  These generally aren't installed/available as static libs anymore.

**Working dynamic build** (run from `src/`):

```sh
g++ -c -Os `fltk-config --cxxflags` Bargraph.cxx
g++ -c -Os `fltk-config --cxxflags` Codebox.cxx
g++ -c -Os `sdl-config --cflags` Cw.cxx
g++ -c -Os `fltk-config --cxxflags` Knob.cxx
fluid -c m.fl
g++ -c -Os `fltk-config --cxxflags` m.cxx
g++ -o m m.o Bargraph.o Codebox.o Cw.o Knob.o \
  `sdl-config --libs` `fltk-config --use-images --ldflags` -ldl
```

**Why `--use-images`:** the About box loads `pix/fltklogo.gif` and
`pix/sdl_button.gif` via `Fl_GIF_Image`/`Fl_Anim_GIF_Image`, which live in
`libfltk_images`, not the core `libfltk`. Without `--use-images`,
linking fails with `undefined reference to Fl_GIF_Image::animate` etc.

Result: `m` binary in `src/`, ~50KB, dynamically linked. Run it with
`./m` from an environment with a display and audio device.

## Known harmless warnings

- `narrowing conversion of 'c' from 'int' to 'char'` in `Codebox.cxx` —
  pre-existing, not a build blocker.
- `'int Fl_Input_::position(int)' is deprecated` in `Codebox.cxx` — FLTK
  1.4 renamed it to `insert_position()`; still works today, not urgent.
