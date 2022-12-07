#ifndef PLATFORM_H
#define PLATFORM_H
#ifdef __MINGW32__
#error "leanocrypt cannot currently be compiled under MinGW. For details, see platform.h"
#include <sys/time.h> 
#define lstat(x,y) stat(x,y)
#define S_ISLNK(x) 0
#define S_IWGRP 0
#define S_IWOTH 0

static inline int fchown(int fd, int owner, int group) {
  return 0;
}
static inline int fchmod(int fd, mode_t mode) {
  return 0;
}
int mkstemp(char *template);

/* leanocrypt.c */

#define ftruncate(fd, len) chsize(fd, len)

/* leanocryptlib.c */

/* avoid conflict with a declaraion in winsock2.h */
#define gethostname(name, len) mingw_compat_gethostname(name, len)
int mingw_compat_gethostname(char *name, size_t len);

int gettimeofday(struct timeval *tv, void *tz);

#endif /* __MINGW32__ */ 

/* ---------------------------------------------------------------------- */
/* setmode() and O_BINARY */

#if defined(__CYGWIN__)

#include <io.h>  /* for setmode() */

#elif defined(__MINGW32__) || defined(__EMX__)

/* nothing needed */

#else /* on a POSIX system, map these to no-ops */

/* BSD defines setmode() with a different meaning, in unistd.h */
static inline void leanocrypt_setmode(int fd, int mode) {
  return;
}
#undef setmode
#define setmode(x,y) leanocrypt_setmode(x,y)

#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

#endif /* __CYGWIN__ || __MINGW32__ || etc */

/* ---------------------------------------------------------------------- */
#endif /* PLATFORM_H */
