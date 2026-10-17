# AirPlayFairPlayPlugin

Out-of-tree native FairPlay media-key plugin for
[AirPlayServer.NET](https://github.com/huiyuanai709/AirPlayServer.NET).

This repository is **GPL-3.0-only** because it wraps PlayFair. The MIT Native
AOT host must never link these objects. Load the built library at runtime with
`--media-key-plugin`.

AirPlay, FairPlay, and Apple are trademarks of Apple Inc. This project is not
affiliated with Apple.

## Host command

`POST /fp-setup` answers only when `AirPlay_FairPlaySetup` is exported and
returns nonzero. A missing plugin, a missing export, or a `0` return stays
HTTP 501. A 72-byte FairPlay `ekey` unwraps only after a successful handshake
has been stored in this process. A new 16-byte setup, or a new process, clears
that handshake.

```text
AirPlayServer.Host.exe --media-key-plugin .\AirPlayFairPlayPlugin.dll --tapclock 127.0.0.1:42125 --screen 480x1040
```

On Linux the same flag points at the shared object:

```text
AirPlayServer.Host --media-key-plugin ./libAirPlayFairPlayPlugin.so --tapclock 127.0.0.1:42125 --screen 480x1040
```

Place the library next to the host, or pass an absolute path. The host calls
the exports with `cdecl` and treats any nonzero return as success.

## C ABI

```c
/* return 1 on success, 0 on failure */
int AirPlay_TryUnwrapContentKey(const uint8_t* wrapped, int wrappedLength, uint8_t* contentKey16);

/* return 1 on success, 0 on failure; write the body length to *written */
int AirPlay_FairPlaySetup(const uint8_t* request, int requestLength,
                          uint8_t* response, int responseCapacity, int* written);
```

`AirPlay_FairPlaySetup` follows the upstream `fp-setup` split:

| Request | Condition | Result |
| --- | --- | --- |
| 16 bytes | `request[4] == 0x03` and mode `request[14]` is 0..3 | 142-byte setup reply, `*written = 142`. Clears any stored handshake. |
| 164 bytes | `request[4] == 0x03` and mode `request[12]` is 0..3 | 32-byte handshake reply, `*written = 32`. Stores the 164-byte key message. |
| anything else | wrong length, version, mode, null pointer, or `responseCapacity` too small | return `0`, `*written = 0` |

`AirPlay_TryUnwrapContentKey` accepts a 72-byte FairPlay `ekey` only when a
handshake is stored (`keymsglen == 164`). On success it writes exactly 16
bytes to `contentKey16` and returns `1`.

v1 keeps that handshake in one process-global struct guarded by a mutex. One
receiver at a time is the supported case.

## Build

CMake produces `AirPlayFairPlayPlugin.dll` on Windows and
`libAirPlayFairPlayPlugin.so` on Linux. The two ABI symbols are exported.
PlayFair symbols stay hidden.

### Linux (gcc or clang)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Clang:

```bash
cmake -S . -B build-clang -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure
```

The shared library is `build/libAirPlayFairPlayPlugin.so`.

### Windows MSVC

From a Visual Studio 2022 x64 developer prompt:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The DLL is `build\Release\AirPlayFairPlayPlugin.dll`. x86 builds use
`src\AirPlayFairPlayPlugin.x86.def` so `GetProcAddress` still finds the
undecorated cdecl names. The Native AOT host is x64.

### Windows clang-cl

From a Visual Studio developer prompt, with `clang-cl` on `PATH`:

```bat
cmake -S . -B build-clang -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure
```

The DLL is `build-clang\AirPlayFairPlayPlugin.dll`.

## Smoke test

`fairplay_plugin_smoke` loads the library by path, the same way the host calls
`NativeLibrary.TryLoad` / `TryGetExport`. It checks the 142-byte and 32-byte
reply sizes, the setup tables, handshake echo, and that a 72-byte unwrap
requires a prior handshake and then returns 16 deterministic bytes. It does
not need an iPhone.

## Layout

```text
include/airplay_fairplay_plugin.h   public cdecl declarations
src/plugin.c                        exported ABI
src/fairplay_session.c              setup / handshake / unwrap session
src/AirPlayFairPlayPlugin.def       MSVC x64 export names
third_party/playfair/               vendored PlayFair (GPL-3.0)
tests/smoke.c                       dlopen harness
```

`third_party/playfair/CMakeLists.txt` is the upstream static-library file and
is not used. This project's top-level CMake compiles those sources into the
shared library directly.

## License

GPL-3.0-only. See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).

Load this plugin only through `--media-key-plugin`. Linking it into
AirPlayServer.NET, or copying PlayFair sources into that MIT tree, is outside
what this repository is for.
