# NOTICE

## PlayFair

`src/AirPlayFairPlay/` is a C# port of the PlayFair interoperability code that
answers AirPlay FairPlay `fp-setup` and unwraps a 72-byte content key.

- Upstream snapshot: <https://github.com/xenos1337/AirPlayServer>
- Commit: `aa9ce92455ac249076d9309bd435985df9cc37e8` (2026-09-26)
- Upstream paths: `AirPlayServerLib/lib/playfair/` and `AirPlayServerLib/lib/fairplay_playfair.c`
- License: GPL-3.0

The port keeps the same setup replies, handshake layout, and decrypt results
as that snapshot on little-endian .NET. An out-of-range mode byte is rejected
before the message-key tables are indexed. `Tables.cs` holds the bytes from
`omg_hax.h`. `unsigned long` table words are stored as `uint` (the low 32 bits
the original x64 build actually uses).

Earlier revisions of this repository also shipped those C sources as a native
plugin. That shared library is no longer the deliverable.

## This repository

The whole tree is GPL-3.0-only. The license text is in `LICENSE`.

AirPlay, FairPlay, and Apple are trademarks of Apple Inc. This project is an
independent interoperability library and is not affiliated with, endorsed by,
or sponsored by Apple.
