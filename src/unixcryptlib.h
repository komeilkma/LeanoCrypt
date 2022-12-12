/* Copyright (C) 2022 Komeil Majidi.
 */
#ifndef _UNIXCRYPTLIB_H
#define _UNIXCRYPTLIB_H
#include "leanocryptlib.h"
int unixcrypt_init(leanocrypt_stream_t *b, const char *key);
int unixcrypt(leanocrypt_stream_t *b);
int unixcrypt_end(leanocrypt_stream_t *b);

#endif /* _UNIXCRYPTLIB_H */
