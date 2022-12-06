/* Copyright (C) 2022 Komeil Majidi. */


#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "leanocryptlib.h"

const char *leanocrypt_error(int st);

int leanoencrypt_streams(FILE *fin, FILE *fout, const char *key);
int leanodencrypt_streams(FILE *fin, FILE *fout, const char *key);
int cckeychange_streams(FILE *fin, FILE *fout, const char *key1, const char *key2);
int unixcrypt_streams(FILE *fin, FILE *fout, const char *key);
int keycheck_stream(FILE *fin, const char *key);

int leanoencrypt_file(int fd, const char *key);
int leanodencrypt_file(int fd, const char *key);
int cckeychange_file(int fd, const char *key1, const char *key2);
int unixcrypt_file(int fd, const char *key);
