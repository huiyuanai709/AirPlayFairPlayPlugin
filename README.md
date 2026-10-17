# AirPlayFairPlayPlugin

纯 .NET 的 FairPlay / PlayFair 实现，许可证为 **GPL-3.0-only**。
本仓库把 [xenos1337/AirPlayServer](https://github.com/xenos1337/AirPlayServer)
里的 `fp-setup` 与 72 字节 `ekey` 解密移植到 C#（.NET 11）。
AirPlay、FairPlay、Apple 是 Apple Inc. 的商标。本项目与 Apple 无关。

Pure .NET FairPlay / PlayFair, licensed **GPL-3.0-only**.
This repository ports `fp-setup` and 72-byte `ekey` decrypt from
[xenos1337/AirPlayServer](https://github.com/xenos1337/AirPlayServer) to C# (.NET 11).
AirPlay, FairPlay, and Apple are trademarks of Apple Inc. This project is not affiliated with Apple.

## 公共 API / Public API

`FairPlaySession` 是一次接收机会话：

| 调用 | 输入 | 成功时 |
| --- | --- | --- |
| `TryRespond` | 16 字节，`request[4] == 0x03`，模式 `request[14]` 为 0..3 | 写入 142 字节 setup 回复，并清掉已保存的握手 |
| `TryRespond` | 164 字节，`request[4] == 0x03`，模式 `request[12]` 为 0..3 | 写入 32 字节握手回复，并保存 164 字节 key message |
| `TryUnwrap` | 72 字节 ekey，且本实例已完成握手 | 写入 16 字节内容密钥 |

失败返回 `false`，`written` 为 0，不改输出缓冲区，也不清掉已经成功的握手。
一次新的 16 字节 setup 会清掉握手，之后 `TryUnwrap` 失败，直到下一次握手。

`FairPlaySession` is one receiver session:

| Call | Input | On success |
| --- | --- | --- |
| `TryRespond` | 16 bytes, `request[4] == 0x03`, mode `request[14]` in 0..3 | writes the 142-byte setup reply and clears any handshake |
| `TryRespond` | 164 bytes, `request[4] == 0x03`, mode `request[12]` in 0..3 | writes the 32-byte handshake reply and stores the 164-byte key message |
| `TryUnwrap` | 72-byte ekey after a handshake on this instance | writes 16 content-key bytes |

Failure returns `false`, sets `written` to 0, leaves the output buffer alone, and does not drop a handshake that already succeeded.
A later 16-byte setup clears that handshake, so `TryUnwrap` fails until the next handshake.

## 如何接到 AirPlayServer.NET / Plugging into AirPlayServer.NET

[AirPlayServer.NET](https://github.com/huiyuanai709/AirPlayServer.NET) 的 Native AOT 宿主
**不能**在运行时加载这个托管程序集。`--media-key-plugin` 只加载原生 DLL。
请把本库编进一个 GPL 宿主，而不是再做一层原生插件。

宿主引用 `AirPlayFairPlay` 和 `AirPlayServer.Core` 后，用进程内转发实现
`IMediaKeyUnwrapper` 与 `IFairPlaySetup`（方法签名与 `FairPlaySession` 相同）：

```csharp
sealed class FairPlayKeys(FairPlaySession session) : IMediaKeyUnwrapper, IFairPlaySetup
{
    public bool TryUnwrap(ReadOnlySpan<byte> wrappedKey, Span<byte> contentKey)
        => session.TryUnwrap(wrappedKey, contentKey);

    public bool TryRespond(ReadOnlySpan<byte> request, Span<byte> response, out int written)
        => session.TryRespond(request, response, out written);
}
```

把这个类型交给 AirPlayServer.NET 里原先挂插件的位置。
**发布出来的宿主二进制是 GPL-3.0**，因为里面包含了本库。
MIT 的 AirPlayServer.NET 仓库本身不要引用本项目。宿主接线留到本库落地之后再做。

The Native AOT host in [AirPlayServer.NET](https://github.com/huiyuanai709/AirPlayServer.NET)
cannot load this managed assembly at runtime. `--media-key-plugin` loads a native DLL only.
Build this library into a GPL host instead of adding another native shim.

After the host references both `AirPlayFairPlay` and `AirPlayServer.Core`, forward
`IMediaKeyUnwrapper` and `IFairPlaySetup` in-process. Those methods match `FairPlaySession`.
Hand that type to the same place the host used to attach the native plugin.
**The published host binary is GPL-3.0** because it contains this library.
The MIT AirPlayServer.NET repository should not reference this project.
Wiring the host is a follow-up after this library lands.

## 构建 / Build

```bash
dotnet test AirPlayFairPlay.slnx -c Release
```

测试不需要 iPhone。它们检查 setup 142 字节、握手 32 字节、状态机，
以及四种模式的解密结果与 PlayFair C 实现一致。

Tests do not need an iPhone. They check the 142-byte setup, the 32-byte handshake,
session state, and decrypt results for modes 0..3 against the PlayFair C implementation.

## 许可证 / License

GPL-3.0-only。见 [LICENSE](LICENSE) 与 [NOTICE.md](NOTICE.md)。
以后公开本仓库没有额外限制；整仓已经按 GPL-3.0 授权。

GPL-3.0-only. See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).
Publishing this repository later is already covered: the whole tree is GPL-3.0.
