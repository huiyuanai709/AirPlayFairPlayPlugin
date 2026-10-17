/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FAIRPLAY_SESSION_H
#define FAIRPLAY_SESSION_H

/* Upstream-compatible results: 0 success, -1 failure. */

int fairplay_session_setup(const unsigned char req[16], unsigned char res[142]);
int fairplay_session_handshake(const unsigned char req[164], unsigned char res[32]);
int fairplay_session_decrypt(const unsigned char input[72], unsigned char output[16]);

#endif
