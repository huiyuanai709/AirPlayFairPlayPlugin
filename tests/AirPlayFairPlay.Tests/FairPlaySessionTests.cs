using AirPlayFairPlay;
using Xunit;

namespace AirPlayFairPlay.Tests;

public class FairPlaySessionTests
{
    private static readonly string[] ContentKeys =
    [
        "28e7e60d2fee72fd6faa7fe1f5e5d65e",
        "c6091d788099f737648caff98b6e38a1",
        "ce29a8432940a1ab9390c40eb65bfa1c",
        "e784babe63a4e32b043fa960dc98b679",
    ];

    private static readonly string[] FlippedKeys =
    [
        "d7e7e60d2fee72fd6faa7fe1f5e5d65e",
        "39091d788099f737648caff98b6e38a1",
        "3129a8432940a1ab9390c40eb65bfa1c",
        "1884babe63a4e32b043fa960dc98b679",
    ];

    private static readonly uint[] SetupFnv =
    [
        0x44e9a1e7,
        0x727c3b96,
        0x977cb4bf,
        0x04f46728,
    ];

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void Setup_writes_142_byte_reply_for_each_mode(int mode)
    {
        var session = new FairPlaySession();
        byte[] request = SetupRequest(mode);
        byte[] response = new byte[4096];
        response.AsSpan(142).Fill(0x5a);

        Assert.True(session.TryRespond(request, response, out int written));
        Assert.Equal(142, written);
        Assert.Equal((byte)'F', response[0]);
        Assert.Equal((byte)'P', response[1]);
        Assert.Equal((byte)'L', response[2]);
        Assert.Equal((byte)'Y', response[3]);
        Assert.Equal((byte)mode, response[13]);
        Assert.Equal(SetupFnv[mode], Fnv1a(response.AsSpan(0, 142)));
        Assert.Equal(0x5a, response[142]);
    }

    [Fact]
    public void Handshake_writes_32_byte_reply_and_echoes_tail()
    {
        var session = new FairPlaySession();
        byte[] request = Handshake(1);
        byte[] response = new byte[64];
        response.AsSpan(32).Fill(0x11);

        Assert.True(session.TryRespond(request, response, out int written));
        Assert.Equal(32, written);
        byte[] header = [0x46, 0x50, 0x4c, 0x59, 0x03, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x14];
        Assert.Equal(header, response.AsSpan(0, 12).ToArray());
        Assert.Equal(request.AsSpan(144, 20).ToArray(), response.AsSpan(12, 20).ToArray());
        Assert.Equal(0x11, response[32]);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void Unwrap_matches_playfair_oracle(int mode)
    {
        var session = HandshakeSession(mode);
        byte[] ekey = Ekey();
        byte[] content = new byte[16];

        Assert.True(session.TryUnwrap(ekey, content));
        Assert.Equal(ContentKeys[mode], Convert.ToHexString(content).ToLowerInvariant());

        ekey[16] ^= 0xff;
        Assert.True(session.TryUnwrap(ekey, content));
        Assert.Equal(FlippedKeys[mode], Convert.ToHexString(content).ToLowerInvariant());
    }

    [Fact]
    public void Unwrap_is_deterministic_and_needs_a_handshake()
    {
        var session = new FairPlaySession();
        byte[] ekey = Ekey();
        byte[] content = new byte[16];
        content.AsSpan().Fill(0xee);

        Assert.False(session.TryUnwrap(ekey, content));
        Assert.Equal(0xee, content[0]);

        Assert.True(session.TryRespond(Handshake(0), new byte[32], out int written));
        Assert.Equal(32, written);
        Assert.True(session.TryUnwrap(ekey, content));
        byte[] again = new byte[16];
        Assert.True(session.TryUnwrap(ekey, again));
        Assert.Equal(content, again);
    }

    [Fact]
    public void Setup_clears_a_stored_handshake()
    {
        var session = HandshakeSession(2);
        byte[] content = new byte[16];
        Assert.True(session.TryUnwrap(Ekey(), content));

        Assert.True(session.TryRespond(SetupRequest(0), new byte[142], out _));
        content.AsSpan().Fill(0xab);
        Assert.False(session.TryUnwrap(Ekey(), content));
        Assert.Equal(0xab, content[0]);
    }

    [Fact]
    public void Rejected_setup_and_handshake_keep_the_stored_key()
    {
        var session = HandshakeSession(3);
        byte[] expected = new byte[16];
        Assert.True(session.TryUnwrap(Ekey(), expected));

        byte[] badVersion = SetupRequest(0);
        badVersion[4] = 0x02;
        byte[] response = new byte[142];
        response.AsSpan().Fill(0x44);
        Assert.False(session.TryRespond(badVersion, response, out int written));
        Assert.Equal(0, written);
        Assert.Equal(0x44, response[0]);

        byte[] badMode = SetupRequest(0);
        badMode[14] = 4;
        Assert.False(session.TryRespond(badMode, response, out written));
        Assert.Equal(0, written);

        byte[] shortSetup = new byte[141];
        Assert.False(session.TryRespond(SetupRequest(1), shortSetup, out written));
        Assert.Equal(0, written);

        byte[] badHandshake = Handshake(1);
        badHandshake[4] = 0x01;
        byte[] handshakeBuffer = new byte[32];
        handshakeBuffer.AsSpan().Fill(0x77);
        Assert.False(session.TryRespond(badHandshake, handshakeBuffer, out written));
        Assert.Equal(0, written);
        Assert.Equal(0x77, handshakeBuffer[0]);

        byte[] shortHandshake = new byte[31];
        Assert.False(session.TryRespond(Handshake(1), shortHandshake, out written));
        Assert.Equal(0, written);

        byte[] still = new byte[16];
        Assert.True(session.TryUnwrap(Ekey(), still));
        Assert.Equal(expected, still);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(15)]
    [InlineData(17)]
    [InlineData(163)]
    [InlineData(165)]
    public void Unexpected_fp_setup_lengths_fail(int length)
    {
        var session = HandshakeSession(0);
        byte[] request = new byte[length];
        if (length > 4)
        {
            request[4] = 0x03;
        }

        Assert.False(session.TryRespond(request, new byte[4096], out int written));
        Assert.Equal(0, written);
        Assert.True(session.TryUnwrap(Ekey(), new byte[16]));
    }

    [Fact]
    public void Unwrap_rejects_the_wrong_ekey_length_without_writing()
    {
        var session = HandshakeSession(0);
        byte[] content = new byte[16];
        content.AsSpan().Fill(0x21);
        Assert.False(session.TryUnwrap(new byte[71], content));
        Assert.False(session.TryUnwrap(new byte[73], content));
        Assert.False(session.TryUnwrap(Ekey(), new byte[15]));
        Assert.Equal(0x21, content[0]);
    }

    [Fact]
    public void Sessions_do_not_share_handshake_state()
    {
        var ready = HandshakeSession(0);
        var other = new FairPlaySession();
        byte[] content = new byte[16];
        content.AsSpan().Fill(0x10);
        Assert.False(other.TryUnwrap(Ekey(), content));
        Assert.Equal(0x10, content[0]);
        Assert.True(ready.TryUnwrap(Ekey(), content));
    }

    private static FairPlaySession HandshakeSession(int mode)
    {
        var session = new FairPlaySession();
        Assert.True(session.TryRespond(Handshake(mode), new byte[32], out int written));
        Assert.Equal(32, written);
        return session;
    }

    private static byte[] SetupRequest(int mode)
    {
        byte[] request = new byte[16];
        request[4] = 0x03;
        request[14] = (byte)mode;
        return request;
    }

    private static byte[] Handshake(int mode)
    {
        byte[] request = new byte[164];
        for (int i = 0; i < request.Length; i++)
        {
            request[i] = (byte)((i * 17) + 3);
        }

        request[0] = 0x46;
        request[1] = 0x50;
        request[2] = 0x4c;
        request[3] = 0x59;
        request[4] = 0x03;
        request[5] = 0x01;
        request[6] = 0x03;
        request[12] = (byte)mode;
        return request;
    }

    private static byte[] Ekey()
    {
        byte[] ekey = new byte[72];
        for (int i = 0; i < ekey.Length; i++)
        {
            ekey[i] = (byte)(0xA0 + i);
        }

        return ekey;
    }

    private static uint Fnv1a(ReadOnlySpan<byte> data)
    {
        uint hash = 2166136261;
        foreach (byte value in data)
        {
            hash ^= value;
            hash *= 16777619;
        }

        return hash;
    }
}
