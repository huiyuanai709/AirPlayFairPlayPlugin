# NOTICE

## PlayFair

This repository vendors the PlayFair interoperability implementation that
answers AirPlay FairPlay `fp-setup` and unwraps a 72-byte content key.

- Upstream snapshot: <https://github.com/xenos1337/AirPlayServer>
- Commit: `aa9ce92455ac249076d9309bd435985df9cc37e8` (2026-09-26)
- Upstream path: `AirPlayServerLib/lib/playfair/`
- Vendored path: `third_party/playfair/`
- License: GPL-3.0 (`third_party/playfair/LICENSE.md`)

`third_party/playfair/omg_hax.h` is line-wrapped so each physical source line
stays within the MSVC source-line limit. The initializer bytes are unchanged
from that snapshot. A comment at the top of the header records the change.

`src/fairplay_session.c` adapts `AirPlayServerLib/lib/fairplay_playfair.c`
from the same snapshot. The upstream `logger_t` argument was only stored and
is omitted here. The four 142-byte setup replies and the 32-byte handshake
layout are the same bytes as that file. An out-of-range mode byte is rejected
before PlayFair indexes `message_key[mode]`.

These sources stay in this repository. They are not added to
[AirPlayServer.NET](https://github.com/huiyuanai709/AirPlayServer.NET).

## This plugin

Code outside `third_party/playfair/` is GPL-3.0-only as well. The full license
text is in `LICENSE`.

AirPlay, FairPlay, and Apple are trademarks of Apple Inc. This project is an
independent interoperability plugin and is not affiliated with, endorsed by,
or sponsored by Apple.
