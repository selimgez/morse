# Cross-compiling morse.exe for Windows (from Linux, via MinGW-w64)

This produced `src/morse.exe`: a static, self-contained x86-64 Windows
binary (~4.3MB) built entirely from this Linux box — no Windows/MSYS2
needed. Read this before redoing it; it took real trial-and-error.

## Why this is harder than the Linux build

Ubuntu's apt has no prebuilt FLTK or SDL2 for the mingw-w64 target
(`apt-cache search mingw | grep -i "fltk\|sdl"` returns nothing). Both had
to be cross-compiled from source into a local sysroot before morse itself
could be built.

## 1. Toolchain

```
apt-get install -y g++-mingw-w64-x86-64 cmake ninja-build wget
update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
```

Use the **posix** thread-model variant, not **win32** — it's the more
compatible default for C++ code that touches `<thread>`/`<mutex>`/etc.
(morse itself doesn't, but there's no reason to use the more restrictive one).

## 2. SDL2, cross-compiled, static

`fltk.org`'s own tarball URLs 404 (`https://www.fltk.org/pub/fltk/...` —
site's download layout has changed); use GitHub release assets instead.

```sh
cd /root/build   # or wherever
wget -q https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz
tar xzf SDL2-2.30.9.tar.gz
cd SDL2-2.30.9 && mkdir build-win64 && cd build-win64
cmake .. -G Ninja \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
  -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
  -DCMAKE_INSTALL_PREFIX=/opt/win64 \
  -DSDL_STATIC=ON -DSDL_SHARED=OFF -DCMAKE_BUILD_TYPE=Release
ninja && ninja install
```

Note: morse only uses the ancient `SDL_OpenAudio`/`SDL_PauseAudio`
single-device API (see `Cw.cxx`), which SDL2 still ships for backward
compat, so no source changes were needed to target SDL2 instead of the
original SDL1.2 — except one: SDL2's `SDL_main.h` `#define`s `main` to
`SDL_main` on Windows unless told not to, which would silently break
linking. Fixed by adding `#define SDL_MAIN_HANDLED` before `#include
<SDL.h>` in `Cw.cxx` (already committed) — harmless no-op on Linux/SDL1.2.

## 3. FLTK, cross-compiled, static — **version 1.3.11 specifically**

Same 404 problem as SDL2: use the GitHub release asset, not fltk.org's
own path.

```sh
wget -q -O fltk-1.3.11-source.tar.gz \
  https://github.com/fltk/fltk/releases/download/release-1.3.11/fltk-1.3.11-source.tar.gz
tar xzf fltk-1.3.11-source.tar.gz
```

**Why 1.3.11, not whatever `fluid`/`fltk-config` apt happens to install:**
this repo's `m.fl` is old FLTK-1.1-era UI description. Feeding it through a
1.4.x `fluid` generates code that calls 1.4-only APIs (e.g.
`Fl_Anim_GIF_Image`, used for the About-box logos) which don't exist in
1.3 headers/libs — this is the *exact* version-mismatch bug hit during the
**Linux** build (see `BUILDING.md` §1: installing `libfltk1.3-dev` pulled
in a 1.4.x `fluid` via its `Recommends`, and the two disagreed). For the
Windows cross-build this bites twice as hard, since fluid can only run
natively (it's a GUI code generator, not something you'd cross-run under
Wine) while the target *library* is cross-compiled — so the native `fluid`
and the cross-compiled library **must be built from the identical FLTK
source tree** or they'll disagree on API surface. Solution: build FLTK
1.3.11 twice from the same source directory — once natively (for a
matching `fluid`), once cross-compiled (for the target library) — rather
than relying on whatever `fluid` a package manager happens to hand you.

**3a. Native build, just to get a matching `fluid`:**

```sh
cd fltk-1.3.11 && mkdir build-native && cd build-native
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPTION_BUILD_SHARED_LIBS=OFF
ninja fluid
# -> build-native/bin/fluid, confirm with: build-native/bin/fluid --version
```

**3b. Cross-compiled build, for the actual Windows library:**

```sh
cd fltk-1.3.11 && mkdir build-win64 && cd build-win64
cmake .. -G Ninja \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
  -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
  -DCMAKE_INSTALL_PREFIX=/opt/win64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPTION_BUILD_SHARED_LIBS=OFF -DOPTION_USE_GL=OFF \
  -DFLTK_BUILD_TEST=OFF -DFLTK_BUILD_EXAMPLES=OFF
ninja fltk fltk_images fltk_forms && ninja install
```

ZLIB/PNG/JPEG all fall back to FLTK's builtin copies automatically (no
system dev packages found for the mingw target, which is fine — one less
thing to cross-compile).

This also installs cross-target `fltk-config`/`sdl2-config` shell scripts
into `/opt/win64/bin` — these run fine under Linux bash (they're just
scripts that print flags), they just point at the mingw compiler/libs.

## 4. Build morse itself

From `src/`, using the **native** fluid (1.3.11) and the cross **install**
of fltk-config/sdl2-config:

```sh
FLUID=/root/build/fltk-1.3.11/build-native/bin/fluid
FLTKCFG=/opt/win64/bin/fltk-config
SDLCFG=/opt/win64/bin/sdl2-config
MGCC=x86_64-w64-mingw32-g++

$FLUID -c m.fl                      # -> m.cxx, m.h
./help.py < m.htm > Help.h          # needs the python3 fix from BUILDING.md

$MGCC -c -Os $($FLTKCFG --cxxflags) Bargraph.cxx
$MGCC -c -Os $($FLTKCFG --cxxflags) Codebox.cxx
$MGCC -c -Os $($SDLCFG --cflags)    Cw.cxx
$MGCC -c -Os $($FLTKCFG --cxxflags) Knob.cxx
$MGCC -c -Os $($FLTKCFG --cxxflags) m.cxx

$MGCC -o morse.exe m.o Bargraph.o Codebox.o Cw.o Knob.o \
  $($SDLCFG --static-libs) \
  $($FLTKCFG --use-images --ldstaticflags) \
  -static -static-libgcc -static-libstdc++
```

Unlike the Linux build, `-static` **works cleanly** here: FLTK's Windows
backend depends only on system DLLs (GDI32, USER32, COMCTL32, OLE32...),
not the X11/cairo/pango/wayland stack that made `-static` impossible on
Linux (see `BUILDING.md` §3). Confirmed self-contained:

```sh
x86_64-w64-mingw32-objdump -p morse.exe | grep "DLL Name"
# -> only ADVAPI32, COMCTL32, GDI32, IMM32, KERNEL32, msvcrt, ole32,
#    OLEAUT32, SETUPAPI, SHELL32, USER32, VERSION, WINMM — all present
#    on any stock Windows install.
```

Also: the About-box logos (`pix/*.gif`) and the help text (`m.htm`) are
both embedded into `m.cxx`/`Help.h` as compiled-in data by `fluid`/
`help.py` at build time (`Fl_Pixmap` arrays, a `HelpString` literal) — not
loaded from disk at runtime. So `morse.exe` is a single portable file;
it doesn't need the `pix/` folder or anything else alongside it.

## Result

`src/morse.exe` — copy it anywhere on a Windows machine and double-click.
No install, no DLLs, no MSYS2 needed on the target machine.
