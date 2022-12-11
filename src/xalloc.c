/* Copyright (C) 2022 Komeil Majidi.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xalloc.h"

/* safe malloc */
void *xalloc(size_t size, const char *myname) {
  void *p = malloc(size);
  if (p==NULL) {
    fprintf(stderr, "%s: %s\n", myname, strerror(errno));
    exit(2);
  }
  return p;
}

/* safe realloc */
void *xrealloc(void *p, size_t size, const char *myname) {
  p = realloc(p, size);
  if (p==NULL) {
    fprintf(stderr, "%s: %s\n", myname, strerror(errno));
    exit(2);
  }
  return p;
}

#define INITSIZE 32
char *xreadline(FILE *fin, const char *myname) {
  int buflen = INITSIZE;
  char *buf = (char *)xalloc(buflen, myname);
  char *res, *nl;

  res = fgets(buf, INITSIZE, fin);
  if (res==NULL) {
    free(buf);
    return NULL;
  }
  nl = strchr(buf, '\n');
  while (nl == NULL) {
    int oldbuflen = buflen;
    buflen <<= 1;
    buf = (char *)xrealloc(buf, buflen, myname);
    res = fgets(buf+oldbuflen-1, buflen-oldbuflen+1, fin);
    if (res==NULL) {
      return buf;
    }
    nl = strchr(buf+oldbuflen-1, '\n');
  }
  /* remove trailing '\n' */
  *nl = 0;
  /* remove trailing '\r', if any */
  if (nl > buf && nl[-1] == '\r') {
    nl[-1] = 0;
  }
  return buf;
}


