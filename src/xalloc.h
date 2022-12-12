/* Copyright (C) 2022 Komeil Majidi.*/
#ifndef __XALLOC_H
#define __XALLOC_H
#include <stdio.h>
/* safe malloc */
void *xalloc(size_t size, const char *myname);

/* safe realloc */
void *xrealloc(void *p, size_t size, const char *myname);

/* read an allocated line from input stream */
char *xreadline(FILE *fin, const char *myname);

#endif /* __XALLOC_H */
