/* Copyright (C) 2022 Komeil Majidi. */


#ifdef HAVE_CONFIG_H
#include <config.h>  /* generated by configure */
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "leanocryptlib.h"
#include "unixcryptlib.h"
#include "platform.h"

#include "gettext.h"
#define _(String) gettext (String)
#define N_(String) gettext_noop (String)

/* heuristically, the fastest inbufsize is 992 - this is slightly, but
   significantly, faster than 1024 or very large buffer sizes. I'm not
   sure why this is - maybe some strange interaction between the
   filesystem blocksize and the page size? */

#define INBUFSIZE 992  
#define MIDBUFSIZE 1024  /* for key change */
#define OUTBUFSIZE 1056

/* ---------------------------------------------------------------------- */
/* leanocrypt error messages. These correspond to the error codes returned
   by the leanocrypt functions. Note: the error code -3 corresponds to an
   i/o error, with errno set, the error code -1 corresponds to any
   other system error, with errno set, and the error code -2
   corresponds to a leanocrypt error, with leanocrypt_errno set. */

/* ---------------------------------------------------------------------- */
/* error handling */

const char *leanocrypt_error(int st) {
  if (st == -1 || st == -3) {
    return strerror(errno);
  } 
  if (st == -2) {
    switch (leanocrypt_errno) {
    case leanocrypt_EFORMAT:
      return _("bad file format");
      break;
    case leanocrypt_EMISMATCH:
      return _("key does not match");
      break;
    case leanocrypt_EBUFFER:
      return _("buffer overflow");
      break;
    default:
      /* do nothing */
      break;
    }
  }
  return _("unknown error");
}

/* ---------------------------------------------------------------------- */

/* leanodencrypt_init_r is a variant of leanodencrypt_init that works around
   bug #1872759. An unlucky combination of a past version of leanocrypt,
   cygwin, and windows accidentally read the key file in binary mode,
   resulting in '\r' to be appended to the end of the key string.
   While the current version of xreadline() strips '\r' from the end
   of keys read from a file, we must still support decryption of these
   legacy files for backward compatibility. To this end,
   leanodencrypt_init_r registers two alternate keys: "key" and "key\r".
   The first matching one is used. */

static int leanodencrypt_init_r(leanocrypt_stream_t *b, const char *key, int flags) {
  char *key2;
  const char *keylist[2];
  int len = strlen(key);
  int r;

  key2 = (char *)malloc(len+2);
  if (!key2) {
    return -1;
  }
  strcpy(key2, key);
  key2[len] = '\r';
  key2[len+1] = 0;

  keylist[0] = key;
  keylist[1] = key2;
  r = leanodencrypt_multi_init(b, 2, keylist, flags);
  free(key2);
  return r;
}


/* ---------------------------------------------------------------------- */
/* keychange = compose decryption and encryption */

struct keychange_state_s {
  leanocrypt_stream_t b1;
  leanocrypt_stream_t b2;
  int iv;                /* count-down for IV bytes */
  char buf[MIDBUFSIZE];
};
typedef struct keychange_state_s keychange_state_t;

static void keychange_state_free(keychange_state_t *st) {
  leanodencrypt_end(&st->b1);
  leanoencrypt_end(&st->b2);
  free(st);
}

static int keychange_init(leanocrypt_stream_t *b, const char *key1, const char *key2) {
  keychange_state_t *st;
  int r;
  int cerr, err;

  st = (keychange_state_t *)malloc(sizeof(keychange_state_t));
  if (st == NULL) {
    return -1;
  }
  b->state = (void *)st;
  st->b1.state = NULL;
  st->b2.state = NULL;

  r = leanodencrypt_init_r(&st->b1, key1, 0);
  if (r) {
    goto error;
  }
  r = leanoencrypt_init(&st->b2, key2);
  if (r) {
    goto error;
  }
  st->b2.next_in = &st->buf[0];
  st->b2.avail_in = 0;
  st->iv = 32;  /* count down IV bytes */

  return 0;

 error:
  cerr = leanocrypt_errno;
  err = errno;
  keychange_state_free(st);
  b->state = NULL;
  leanocrypt_errno = cerr;
  errno = err;
  return r;
}

static int keychange(leanocrypt_stream_t *b) {
  keychange_state_t *st = (keychange_state_t *)b->state;
  int r;
  int cerr, err;

  /* note: we do not write anything until we have seen 32 bytes of
     input. This way, we don't write the output IV until the input IV
     has been verified. */

  while (1) { 
    /* clear mid-buffer */
    if (b->avail_out && !st->iv) {
      st->b2.next_out = b->next_out;
      st->b2.avail_out = b->avail_out;
      r = leanoencrypt(&st->b2);
      if (r) {
	goto error;
      }
      b->next_out = st->b2.next_out;
      b->avail_out = st->b2.avail_out;
    }
    
    /* if mid-buffer not empty, or no input available, stop */
    if (st->b2.avail_in != 0 || b->avail_in == 0) {
      break;
    }

    /* fill mid-buffer */
    st->b1.next_out = &st->buf[0];
    st->b1.avail_out = MIDBUFSIZE;
    st->b1.next_in = b->next_in;
    st->b1.avail_in = b->avail_in;
    r = leanodencrypt(&st->b1);
    if (r) {
      goto error;
    }
    if (st->iv) {
      st->iv -= b->avail_in - st->b1.avail_in;
      if (st->iv <= 0) {
	st->iv = 0;
      }
    }
    b->next_in = st->b1.next_in;
    b->avail_in = st->b1.avail_in;
    st->b2.next_in = &st->buf[0];
    st->b2.avail_in = st->b1.next_out - st->b2.next_in;
  }
  return 0;

 error:
  cerr = leanocrypt_errno;
  err = errno;
  keychange_state_free(st);
  b->state = NULL; /* guard against double free */
  leanocrypt_errno = cerr;
  errno = err;
  return r;
}

static int keychange_end(leanocrypt_stream_t *b) {
  keychange_state_t *st = (keychange_state_t *)b->state;
  int r;
  int cerr, err;

  if (st == NULL) {
    return 0;
  }
  r = leanodencrypt_end(&st->b1);
  if (r) {
    goto error;
  }
  r = leanoencrypt_end(&st->b2);
  if (r) {
    goto error;
  }
  free(b->state);
  b->state = NULL;
  return 0;

 error:
  cerr = leanocrypt_errno;
  err = errno;
  keychange_state_free(st);
  b->state = NULL;
  leanocrypt_errno = cerr;
  errno = err;
  return r;
}

/* ---------------------------------------------------------------------- */
/* encryption/decryption of streams */

typedef int initfun(leanocrypt_stream_t *b, const char *key);
typedef int workfun(leanocrypt_stream_t *b);
typedef int endfun(leanocrypt_stream_t *b);

/* apply leanocrypt_stream to pipe stuff from fin to fout. Assume the
   leanocrypt_stream has already been initialized. */
static int streamhandler(leanocrypt_stream_t *b, workfun *work, endfun *end, 
			 FILE *fin, FILE *fout) {
  /* maybe should align buffers on page boundary */
  char inbuf[INBUFSIZE], outbuf[OUTBUFSIZE]; 
  int eof = 0;
  int r;
  int cerr, err;
  int n;

  clearerr(fin);

  b->avail_in = 0;

  while (1) {
    /* fill input buffer */
    if (b->avail_in == 0 && !eof) {
      r = fread(inbuf, 1, INBUFSIZE, fin);
      b->next_in = &inbuf[0];
      b->avail_in = r;
      if (r<INBUFSIZE) {
	eof = 1;
      }
      if (ferror(fin)) {
	r = -3;
	goto error;
      }
    }
    /* prepare output buffer */
    b->next_out = &outbuf[0];
    b->avail_out = OUTBUFSIZE;

    /* do some work */
    r = work(b);
    if (r) {
      return r;
    }
    /* process output buffer */
    if (b->avail_out < OUTBUFSIZE) {
      n = OUTBUFSIZE - b->avail_out;
      r = fwrite(outbuf, 1, n, fout);
      if (r < n) {
	r = -3;
	goto error;
      }
    }
    if (eof && b->avail_out != 0) {
      break;
    }
  }
  r = end(b);
  if (r) {
    return r;
  }
  return 0;

 error:
  err = errno;
  cerr = leanocrypt_errno;
  end(b);
  errno = err;
  leanocrypt_errno = cerr;
  return r;
}  

int leanoencrypt_streams(FILE *fin, FILE *fout, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = leanoencrypt_init(b, key);
  if (r) {
    return r;
  }

  return streamhandler(b, leanoencrypt, leanoencrypt_end, fin, fout);
}

int leanodencrypt_streams(FILE *fin, FILE *fout, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;
  int flags = 0;

  if (cmd.mismatch) {
    flags |= leanocrypt_MISMATCH;
  }

  r = leanodencrypt_init_r(b, key, flags);
  if (r) {
    return r;
  }

  return streamhandler(b, leanodencrypt, leanodencrypt_end, fin, fout);
}

int cckeychange_streams(FILE *fin, FILE *fout, const char *key1, const char *key2) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = keychange_init(b, key1, key2);
  if (r) {
    return r;
  }

  return streamhandler(b, keychange, keychange_end, fin, fout);
}

int unixcrypt_streams(FILE *fin, FILE *fout, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = unixcrypt_init(b, key);
  if (r) {
    return r;
  }

  return streamhandler(b, unixcrypt, unixcrypt_end, fin, fout);
}

/* check if the key matches the given file. This is done by decrypting
   the first 32 bytes, ignoring the output. Return 0 on success, -1 on
   error with errno set, -2 on error with leanocrypt_errno set. */
int keycheck_stream(FILE *fin, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;
  char inbuf[32];
  char outbuf[32];
  int cerr, err;

  clearerr(fin);

  r = leanodencrypt_init_r(b, key, 0);
  if (r) {
    return r;
  }

  /* fill input buffer */
  r = fread(inbuf, 1, 32, fin);
  if (ferror(fin)) {
    r = -3;
    goto error;
  }
  b->next_in = &inbuf[0];
  b->avail_in = r;
  b->next_out = &outbuf[0];
  b->avail_out = 32;
  
  /* try to decrypt */
  r = leanodencrypt(b);
  if (r) {
    return r;
  }
  r = leanodencrypt_end(b);
  if (r) {
    return r;
  }
  return 0;
 
 error:
  err = errno;
  cerr = leanocrypt_errno;
  leanodencrypt_end(b);
  errno = err;
  leanocrypt_errno = cerr;
  return r;
}

/* ---------------------------------------------------------------------- */
/* destructive encryption/decryption of files */

/* A large value of FILEINBUFSIZE keeps the cost of "lseek" down. It
   is okay for FILEINBUFSIZE to be much larger than MIDBUFSIZE.
   FILEOUTBUFSIZE must be large enough to hold the encryption of
   FILEINBUFSIZE in one piece, or otherwise there will be a buffer
   overflow error. */

#define FILEINBUFSIZE 10240
#define FILEOUTBUFSIZE (FILEINBUFSIZE+32)

/* apply leanocrypt_stream to destructively update (and resize) the given
   fd, which must be opened in read/write mode and seekable.
   Encryption will begin at the current file position (normally 0),
   and extend until the end of the file. Note: this only works if the
   stream encoder b/work/end expands its input by at most
   FILEINBUFSIZE bytes; otherwise there will be a buffer overflow
   error. */

static int filehandler(leanocrypt_stream_t *b, workfun *work, endfun *end,
		       int fd) {
  /* rp = reader's position, wp = writer's position, fp = file position */
  int p;      /* rp-wp */
  char inbuf[FILEINBUFSIZE];
  char outbuf[FILEINBUFSIZE+32];
  int inbufsize, outbufsize;
  off_t offs;
  int r;
  int i;
  int eof=0;
  int err, cerr;

  p = 0;
  outbufsize = 0;

  while (1) {
    /* file is at position wp */
    if (p != 0) {
      r = lseek(fd, p, SEEK_CUR);
      if (r == -1) {
	r = -3;
	goto error;
      }
    }
    /* file is at position rp */

    /* read block */
    i = 0;
    while (i<FILEINBUFSIZE && !eof) {
      r = read(fd, inbuf+i, FILEINBUFSIZE-i);
      if (r == -1) {
	r = -3;
	goto error;
      } else if (r==0) {
	eof = 1;
      }
      i += r;
    }
    p += i;
    inbufsize = i;

    /* file is at position rp */
    if (p != 0) {
      r = lseek(fd, -p, SEEK_CUR);
      if (r == -1) {
	r = -3;
	goto error;
      }
    }
    /* file is at position wp */

    /* write previous block */
    if (outbufsize > p && !eof) {
      leanocrypt_errno = leanocrypt_EBUFFER; /* buffer overflow; should never happen */
      r = -2;
      goto error;
    }
    if (outbufsize != 0) {
      i = 0;
      while (i<outbufsize) {
	r = write(fd, outbuf+i, outbufsize-i);
	if (r == -1) {
	  r = -3;
	  goto error;
	}
	i += r;
      }
      p -= outbufsize;
      outbufsize = 0;
    }
    
    /* encrypt block */
    b->next_in = inbuf;
    b->avail_in = inbufsize;
    b->next_out = outbuf;
    b->avail_out = FILEINBUFSIZE+32;

    r = work(b);
    if (r) {
      return r;
    }      
    
    if (b->avail_in != 0) {
      leanocrypt_errno = leanocrypt_EBUFFER; /* buffer overflow; should never happen */
      r = -2;
      goto error;
    }
    inbufsize = 0;
    outbufsize = FILEINBUFSIZE+32-b->avail_out;

    if (eof && outbufsize == 0) { /* done */
      break;
    }
  }
  /* file is at position wp */

  /* close the stream (we need to do this before truncating, because
     there might be an error!) */
  r = end(b);
  if (r) {
    return r;
  }

  /* truncate the file to where it's been written */
  r = offs = lseek(fd, 0, SEEK_CUR);
  if (r == -1) {
    return -1;
  }
  r = ftruncate(fd, offs);
  if (r == -1) {
    return -1;
  }
  
  return 0;

 error:
  err = errno;
  cerr = leanocrypt_errno;
  end(b);
  errno = err;
  leanocrypt_errno = cerr;
  return r;
}

int leanoencrypt_file(int fd, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = leanoencrypt_init(b, key);
  if (r) {
    return r;
  }

  return filehandler(b, leanoencrypt, leanoencrypt_end, fd);
}

int leanodencrypt_file(int fd, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = leanodencrypt_init_r(b, key, 0);
  if (r) {
    return r;
  }

  return filehandler(b, leanodencrypt, leanodencrypt_end, fd);
}

int cckeychange_file(int fd, const char *key1, const char *key2) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = keychange_init(b, key1, key2);
  if (r) {
    return r;
  }

  return filehandler(b, keychange, keychange_end, fd);
}

int unixcrypt_file(int fd, const char *key) {
  leanocrypt_stream_t ccs;
  leanocrypt_stream_t *b = &ccs;
  int r;

  r = unixcrypt_init(b, key);
  if (r) {
    return r;
  }

  return filehandler(b, unixcrypt, unixcrypt_end, fd);
}
