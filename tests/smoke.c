/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * Loads the plugin the way NativeMediaKeyModule does (dlopen / LoadLibrary)
 * and checks setup and handshake sizes. No Apple device is required.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

typedef int (*unwrap_fn)(const uint8_t *wrapped, int wrappedLength, uint8_t *contentKey16);
typedef int (*setup_fn)(const uint8_t *request, int requestLength, uint8_t *response, int responseCapacity, int *written);

static int g_failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_failures++;
    }
}

static uint32_t fnv1a(const uint8_t *data, int length)
{
    uint32_t hash = 2166136261u;
    int i;
    for (i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static void fill_setup(uint8_t req[16], unsigned mode)
{
    static const uint8_t base[16] = {
        0x46, 0x50, 0x4c, 0x59, 0x03, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0xbb
    };
    memcpy(req, base, 16);
    req[14] = (uint8_t)mode;
}

static void fill_handshake(uint8_t req[164], unsigned mode)
{
    int i;
    for (i = 0; i < 164; i++) {
        req[i] = (uint8_t)(i * 17 + 3);
    }
    req[0] = 0x46;
    req[1] = 0x50;
    req[2] = 0x4c;
    req[3] = 0x59;
    req[4] = 0x03;
    req[5] = 0x01;
    req[6] = 0x03;
    req[12] = (uint8_t)mode;
}

#if defined(_WIN32)
static void *open_plugin(const char *path)
{
    return (void *)LoadLibraryA(path);
}

static void *plugin_symbol(void *library, const char *name)
{
    FARPROC proc = GetProcAddress((HMODULE)library, name);
    void *symbol = NULL;
    memcpy(&symbol, &proc, sizeof symbol);
    return symbol;
}

static void close_plugin(void *library)
{
    FreeLibrary((HMODULE)library);
}

static const char *plugin_error(void)
{
    return "LoadLibrary/GetProcAddress failed";
}
#else
static void *open_plugin(const char *path)
{
    return dlopen(path, RTLD_NOW);
}

static void *plugin_symbol(void *library, const char *name)
{
    return dlsym(library, name);
}

static void close_plugin(void *library)
{
    dlclose(library);
}

static const char *plugin_error(void)
{
    const char *err = dlerror();
    return err ? err : "dlopen failed";
}
#endif

int main(int argc, char **argv)
{
    void *library;
    setup_fn setup = NULL;
    unwrap_fn unwrap = NULL;
    uint8_t request[164];
    uint8_t response[256];
    uint8_t key_a[16];
    uint8_t key_b[16];
    uint8_t ekey[72];
    int written;
    int i;
    static const uint32_t k_reply_fnv[4] = {
        0x44e9a1e7u, 0x727c3b96u, 0x977cb4bfu, 0x04f46728u
    };
    static const uint8_t k_header[12] = {
        0x46, 0x50, 0x4c, 0x59, 0x03, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x14
    };

    if (argc != 2) {
        fprintf(stderr, "usage: %s <AirPlayFairPlayPlugin shared library>\n", argv[0]);
        return 2;
    }

    library = open_plugin(argv[1]);
    if (!library) {
        fprintf(stderr, "FAIL: could not load %s (%s)\n", argv[1], plugin_error());
        return 1;
    }

    /* memcpy keeps this an object-pointer copy. A direct cast is not valid ISO C. */
    {
        void *setup_symbol = plugin_symbol(library, "AirPlay_FairPlaySetup");
        void *unwrap_symbol = plugin_symbol(library, "AirPlay_TryUnwrapContentKey");
        _Static_assert(sizeof setup_symbol == sizeof setup, "dlsym and cdecl pointers differ in width");
        memcpy(&setup, &setup_symbol, sizeof setup);
        memcpy(&unwrap, &unwrap_symbol, sizeof unwrap);
    }
    expect(setup != NULL, "AirPlay_FairPlaySetup is exported");
    expect(unwrap != NULL, "AirPlay_TryUnwrapContentKey is exported");
    expect(plugin_symbol(library, "playfair_decrypt") == NULL, "playfair_decrypt stays hidden");
    expect(plugin_symbol(library, "fairplay_session_setup") == NULL, "session helpers stay hidden");
    if (!setup || !unwrap) {
        close_plugin(library);
        return 1;
    }

    memset(ekey, 0, sizeof ekey);
    memset(key_a, 0x5a, sizeof key_a);
    expect(unwrap(ekey, 72, key_a) == 0, "unwrap before handshake fails");
    expect(key_a[0] == 0x5a, "failed unwrap leaves the output buffer untouched");

    for (i = 0; i < 4; i++) {
        fill_setup(request, (unsigned)i);
        memset(response, 0xa5, sizeof response);
        written = 99;
        expect(setup(request, 16, response, (int)sizeof response, &written) == 1, "setup succeeds");
        expect(written == 142, "setup writes 142 bytes");
        expect(response[0] == 0x46 && response[1] == 0x50 && response[2] == 0x4c && response[3] == 0x59,
               "setup reply starts with FPLY");
        expect(response[4] == 0x03 && response[13] == (uint8_t)i, "setup reply carries the requested mode");
        expect(fnv1a(response, 142) == k_reply_fnv[i], "setup reply matches the PlayFair table");
        expect(response[142] == 0xa5, "setup does not write past 142 bytes");
    }

    written = 99;
    memset(response, 0xa5, sizeof response);
    fill_setup(request, 1);
    expect(setup(request, 16, response, 141, &written) == 0, "setup rejects a short buffer");
    expect(written == 0, "short setup sets *written to 0");
    expect(response[0] == 0xa5, "short setup does not write a reply");

    request[4] = 0x02;
    written = 99;
    expect(setup(request, 16, response, 142, &written) == 0, "setup rejects a non-0x03 version");
    expect(written == 0, "bad version sets *written to 0");

    fill_setup(request, 4);
    expect(setup(request, 16, response, 142, &written) == 0, "setup rejects mode 4");

    fill_handshake(request, 3);
    memset(response, 0xa5, sizeof response);
    written = 99;
    expect(setup(request, 164, response, (int)sizeof response, &written) == 1, "handshake succeeds");
    expect(written == 32, "handshake writes 32 bytes");
    expect(memcmp(response, k_header, 12) == 0, "handshake reply starts with the fp header");
    expect(memcmp(response + 12, request + 144, 20) == 0, "handshake reply echoes the last 20 request bytes");
    expect(response[32] == 0xa5, "handshake does not write past 32 bytes");

    for (i = 0; i < 72; i++) {
        ekey[i] = (uint8_t)(0xa0 + i);
    }
    memset(key_a, 0, sizeof key_a);
    memset(key_b, 0, sizeof key_b);
    expect(unwrap(ekey, 72, key_a) == 1, "unwrap succeeds after handshake");
    expect(unwrap(ekey, 72, key_b) == 1, "unwrap is repeatable");
    expect(memcmp(key_a, key_b, 16) == 0, "unwrap is deterministic");

    ekey[16] ^= 0xff;
    expect(unwrap(ekey, 72, key_b) == 1, "unwrap accepts a second ekey");
    expect(memcmp(key_a, key_b, 16) != 0, "a changed ekey changes the content key");
    ekey[16] ^= 0xff;

    expect(unwrap(ekey, 71, key_b) == 0, "unwrap rejects a short ekey");
    expect(unwrap(ekey, 73, key_b) == 0, "unwrap rejects a long ekey");
    expect(unwrap(NULL, 72, key_b) == 0, "unwrap rejects a null ekey");
    expect(unwrap(ekey, 72, NULL) == 0, "unwrap rejects a null output");

    written = 99;
    expect(setup(request, 164, response, 31, &written) == 0, "handshake rejects a short buffer");
    expect(written == 0, "short handshake sets *written to 0");
    expect(unwrap(ekey, 72, key_b) == 1, "a short handshake buffer keeps the previous session");
    expect(memcmp(key_a, key_b, 16) == 0, "session key is unchanged after a short handshake");

    request[4] = 0x01;
    written = 77;
    expect(setup(request, 164, response, 32, &written) == 0, "handshake rejects a non-0x03 version");
    expect(unwrap(ekey, 72, key_b) == 1, "a rejected handshake keeps the previous session");

    request[4] = 0x03;
    request[12] = 4;
    expect(setup(request, 164, response, 32, &written) == 0, "handshake rejects mode 4");
    expect(unwrap(ekey, 72, key_b) == 1, "a rejected mode keeps the previous session");

    fill_setup(request, 2);
    expect(setup(request, 16, response, 142, &written) == 1, "a later setup still succeeds");
    expect(unwrap(ekey, 72, key_b) == 0, "setup clears handshake state");

    fill_handshake(request, 0);
    expect(setup(request, 164, response, 32, &written) == 1, "handshake can run again after setup");
    expect(unwrap(ekey, 72, key_b) == 1, "unwrap works after the second handshake");

    written = 5;
    expect(setup(request, 15, response, 256, &written) == 0, "length 15 fails");
    expect(setup(request, 17, response, 256, &written) == 0, "length 17 fails");
    expect(setup(request, 163, response, 256, &written) == 0, "length 163 fails");
    expect(setup(request, 165, response, 256, &written) == 0, "length 165 fails");
    expect(setup(request, 0, response, 256, &written) == 0, "length 0 fails");
    expect(setup(NULL, 16, response, 256, &written) == 0, "null request fails");
    expect(setup(request, 16, NULL, 256, &written) == 0, "null response fails");
    expect(setup(request, 16, response, 256, NULL) == 0, "null written fails");

    close_plugin(library);
    if (g_failures) {
        fprintf(stderr, "%d failure(s)\n", g_failures);
        return 1;
    }
    printf("ok\n");
    return 0;
}
