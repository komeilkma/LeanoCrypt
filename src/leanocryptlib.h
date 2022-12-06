/* Copyright (C) 2022 Komeil Majidi. */


#ifndef _leanocryptLIB_H
#define _leanocryptLIB_H

#ifdef __cplusplus
extern "C" {
#endif

struct leanocrypt_stream_s {
  char          *next_in;  /* next input byte */
  unsigned int  avail_in;  /* number of bytes available at next_in */

  char          *next_out; /* next output byte should be put there */
  unsigned int  avail_out; /* remaining free space at next_out */

  void *state;             /* internal state, not visible by applications */
};
typedef struct leanocrypt_stream_s leanocrypt_stream_t;

int leanoencrypt_init(leanocrypt_stream_t *b, const char *key);
int leanoencrypt     (leanocrypt_stream_t *b);
int leanoencrypt_end (leanocrypt_stream_t *b);

int leanodencrypt_init(leanocrypt_stream_t *b, const char *key, int flags);
int leanodencrypt     (leanocrypt_stream_t *b);
int leanodencrypt_end (leanocrypt_stream_t *b);


int leanodencrypt_multi_init(leanocrypt_stream_t *b, int n, const char **keylist, int flags);

/* errors */

#define leanocrypt_EFORMAT   1          /* bad file format */
#define leanocrypt_EMISMATCH 2          /* key does not match */
#define leanocrypt_EBUFFER   3          /* buffer overflow */

/* flags */

#define leanocrypt_MISMATCH  1          /* ignore non-matching key */

extern int leanocrypt_errno;

#ifdef  __cplusplus
} // end of extern "C"
#endif

#endif /* _leanocryptLIB_H */
