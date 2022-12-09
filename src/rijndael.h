/* Copyright (C) 2022 Komeil Majidi.*/


#ifndef __RIJNDAEL_H
#define __RIJNDAEL_H

#ifdef HAVE_CONFIG_H
#include <config.h>	
#endif

typedef unsigned char xword8;

#ifdef UINT32_TYPE
typedef UINT32_TYPE xword32;
#else
typedef int xword32;		/* should be a 32-bit integer type */
#endif

union xword8x4_u {
  xword8 w8[4];
  xword32 w32;
};
typedef union xword8x4_u xword8x4;

#include "tables.h"

#define MAXBC		(256/32)
#define MAXKC		(256/32)
#define MAXROUNDS	14
#define MAXRK           ((MAXROUNDS+1)*MAXBC)

typedef struct {
  int BC;
  int KC;
  int ROUNDS;
  int shift[2][4];
  xword32 rk[MAXRK];
} roundkey;

int xrijndaelKeySched(xword32 key[], int keyBits, int blockBits,
		      roundkey *rkk);
void xrijndaelEncrypt(xword32 block[], roundkey *rkk);
void xrijndaelDecrypt(xword32 block[], roundkey *rkk);

#endif				/* __RIJNDAEL_H */
