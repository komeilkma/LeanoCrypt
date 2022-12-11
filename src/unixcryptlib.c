/* Copyright (C) 2022 Komeil Majidi.
 */
#define _XOPEN_SOURCE 500
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>  /* for crypt() */
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>  /* generated by configure */
#endif

#include "unixcryptlib.h"
#include "leanocryptlib.h"

#ifndef HAVE_LIBCRYPT    /* use the dropin replacement for crypt(3) */
  #include "unixcrypt3.h"
  #define crypt crypt_replacement
#endif

#define ascii_to_bin_sun(c) ((c)>'Z'?(c-59):(c)>'9'?((c)-53):(c)-'.')
#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

static char *crypt_sun(const char *key, const char *salt) {
  char salt1[3];
  static char result[13];
  char *p;
  int tmp;

  tmp = 0x3f & ascii_to_bin_sun(salt[0]);
  salt1[0] = bin_to_ascii(tmp);
  tmp = 0x3f & ascii_to_bin_sun(salt[1]);
  salt1[1] = bin_to_ascii(tmp);
  salt1[2] = 0;

  p = crypt(key, salt1);
  if (!p) {
    return NULL;
  }
  strncpy(result, p, 13);

  result[0] = salt[0];
  result[1] = salt[1];

  return result;
}

/* state of the encryption engine */
struct unixcrypt_state_s {
  char box1[0x100];
  char box2[0x100];
  char box3[0x100];
  int j;
  int k;
};
typedef struct unixcrypt_state_s unixcrypt_state;

/* initialize state of encryption engine */
int unixcrypt_init(leanocrypt_stream_t *b, const char *key) {
  unixcrypt_state *st;
  signed char buf[16];
  int i, j, k, acc, tmp, v;
  char *p;

  st = (unixcrypt_state *)malloc(sizeof(unixcrypt_state));
  if (!st) {
    b->state = NULL;
    return -1;
  }
  b->state = (void *)st;

  memset(buf, 0, 16);
  strncpy((char *)buf, key, 8);
  
  buf[8] = buf[0];
  buf[9] = buf[1];
  p = crypt_sun((char *)buf, (char *)&buf[8]);
  if (!p) {
    return -1;
  }

  strncpy((char *)buf, p, 13);
  
  acc = 0x7b;
  
  for (i=0; i<13; i++) {
    acc = i + acc * buf[i];
  }
  
  for (i = 0; i < 0x100; i++) {
    st->box1[i] = i;
    st->box2[i] = 0;
  }
  
  for (i = 0; i < 0x100; i++) {
    acc *= 5;
    acc += buf[i % 13];
    
    v = acc - 0xfff1 * (acc / 0xfff1);
    
    j = (v & 0xff) % (0x100-i);
    k = 0xff - i;
    
    tmp = st->box1[k];
    st->box1[k] = st->box1[j];
    st->box1[j] = tmp;

    if (st->box2[k]==0) {
      j = ((v >> 8) & 0xff) % k;

      while (st->box2[j] != 0) {
	j = (j+1) % k;
      }

      st->box2[k] = j;
      st->box2[j] = k;
    }
  }

  /* calculate box3, the inverse of box1 */
  for (i = 0; i < 0x100; i++) {
    st->box3[st->box1[i] & 0xff] = i;
  }

  /* initialize character count */
  st->j = st->k = 0;

  return 0;
}

/* encrypt/decrypt one character */
static inline int unixcrypt_char(unixcrypt_state *st, int c) {
  c += st->k;
  c = st->box1[c & 0xff];
  c += st->j;
  c = st->box2[c & 0xff];
  c -= st->j;
  c = st->box3[c & 0xff];
  c -= st->k;

  st->k++;
  if (st->k == 0x100) {
    st->j++;
    st->k = 0;
    if (st->j == 0x100) {
      st->j = 0;
    }
  }
  return c;
}

/* encrypt/decrypt buffer */
int unixcrypt(leanocrypt_stream_t *b) {
  int c, cc;

  while (b->avail_in && b->avail_out) {
    c = *(char *)b->next_in;
    b->next_in++;
    b->avail_in--;
    cc = unixcrypt_char((unixcrypt_state *)b->state, c);
    *(char *)b->next_out = cc;
    b->next_out++;
    b->avail_out--;
  }
  return 0;
}

int unixcrypt_end(leanocrypt_stream_t *b) {
  free(b->state);
  b->state = NULL;
  return 0;
}
