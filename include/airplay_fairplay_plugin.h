/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AIRPLAY_FAIRPLAY_PLUGIN_H
#define AIRPLAY_FAIRPLAY_PLUGIN_H

/*
 * C ABI loaded by AirPlayServer.NET NativeMediaKeyModule.
 * Calling convention is cdecl. A nonzero return means success.
 *
 * int AirPlay_TryUnwrapContentKey(const uint8_t* wrapped, int wrappedLength, uint8_t* contentKey16);
 * int AirPlay_FairPlaySetup(const uint8_t* request, int requestLength, uint8_t* response, int responseCapacity, int* written);
 */

#include <stdint.h>

#if defined(_WIN32)
#  if defined(_MSC_VER)
/* MSVC and clang-cl export undecorated names through the module-definition file. */
#    define AIRPLAY_FP_API
#  else
#    define AIRPLAY_FP_API __declspec(dllexport)
#  endif
#  define AIRPLAY_FP_CALL __cdecl
#else
#  define AIRPLAY_FP_API __attribute__((visibility("default")))
#  define AIRPLAY_FP_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

AIRPLAY_FP_API int AIRPLAY_FP_CALL AirPlay_TryUnwrapContentKey(
    const uint8_t *wrapped,
    int wrappedLength,
    uint8_t *contentKey16);

AIRPLAY_FP_API int AIRPLAY_FP_CALL AirPlay_FairPlaySetup(
    const uint8_t *request,
    int requestLength,
    uint8_t *response,
    int responseCapacity,
    int *written);

#ifdef __cplusplus
}
#endif

#endif
