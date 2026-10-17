/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * Runtime media-key plugin for AirPlayServer.NET.
 * The MIT Native AOT host loads this shared library with --media-key-plugin.
 * It does not link these objects.
 */
#include "airplay_fairplay_plugin.h"

#include "fairplay_session.h"

#include <string.h>

static int fail_setup(int *written)
{
    if (written) {
        *written = 0;
    }
    return 0;
}

AIRPLAY_FP_API int AIRPLAY_FP_CALL AirPlay_FairPlaySetup(
    const uint8_t *request,
    int requestLength,
    uint8_t *response,
    int responseCapacity,
    int *written)
{
    if (!written) {
        return 0;
    }
    *written = 0;
    if (!request || !response || requestLength < 0 || responseCapacity < 0) {
        return 0;
    }

    /* Lengths match raop_handler_fpsetup: 16-byte setup, 164-byte handshake. */
    if (requestLength == 16) {
        unsigned char reply[142];
        if (responseCapacity < 142) {
            return 0;
        }
        if (fairplay_session_setup(request, reply) != 0) {
            return 0;
        }
        memcpy(response, reply, 142);
        *written = 142;
        return 1;
    }

    if (requestLength == 164) {
        unsigned char reply[32];
        if (responseCapacity < 32) {
            return 0;
        }
        if (fairplay_session_handshake(request, reply) != 0) {
            return 0;
        }
        memcpy(response, reply, 32);
        *written = 32;
        return 1;
    }

    return fail_setup(written);
}

AIRPLAY_FP_API int AIRPLAY_FP_CALL AirPlay_TryUnwrapContentKey(
    const uint8_t *wrapped,
    int wrappedLength,
    uint8_t *contentKey16)
{
    unsigned char key[16];
    int rc;

    if (!wrapped || !contentKey16 || wrappedLength != 72) {
        return 0;
    }

    memset(key, 0, sizeof key);
    rc = fairplay_session_decrypt(wrapped, key);
    if (rc != 0) {
        memset(key, 0, sizeof key);
        return 0;
    }

    memcpy(contentKey16, key, 16);
    memset(key, 0, sizeof key);
    return 1;
}
