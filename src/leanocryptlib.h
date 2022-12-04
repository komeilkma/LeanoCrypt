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

/* 
   The application may update next_in and avail_in when avail_in has
   dropped to zero. It must update next_out and avail_out when
   avail_out has dropped to zero. All other fields are set by the
   compression library and must not be updated by the application.
*/

int leanoencrypt_init(leanocrypt_stream_t *b, const char *key);
int leanoencrypt     (leanocrypt_stream_t *b);
int leanoencrypt_end (leanocrypt_stream_t *b);

int leanodencrypt_init(leanocrypt_stream_t *b, const char *key, int flags);
int leanodencrypt     (leanocrypt_stream_t *b);
int leanodencrypt_end (leanocrypt_stream_t *b);

/* The interface for encryption and decryption is the same. The
   application must first call the respective init function to
   initialize the internal state. Then it calls the encrypt/decrypt
   function repeatedly, as follows: next_in and next_out must point to
   valid, non-overlapping regions of memory of size at least avail_in
   and avail_out, respectively. Avail_out must be non-zero. Avail_in
   may be zero to retrieve some pending output when no input is
   available, for instance, in an interactive application or at the
   end of stream. Otherwise, avail_in should be non-zero. 

   The encryption/decryption function will read and process as many
   input bytes as possible as long as there is room in the output
   buffer. It will update next_in, avail_in, next_out, and avail_out
   accordingly. It will always flush as much output as possible.
   However, it is possible that some input bytes produce no output,
   because some part of the input may be used for internal purposes.
   Conversely, it is possible that output is produced without reading
   any input. When the encryption/decryption function returns, at
   least one of avail_in or avail_out is 0. If avail_out is non-zero
   after a call, the application may conclude that no more output is
   pending.

   Finally, the internal state must be freed by calling the respective
   end function. This function will discard any unprocessed input
   and/or output, so it should only be called after the application
   has retrieved any pending output. Note: the call to the end
   function should be checked for errors, because this is the only
   place where certain format errors (like a file that is too short)
   are detected.

   All functions return 0 on success, or -1 on error with errno set,
   or -2 on error with leanocrypt_errno set. The _end functions do not
   return -1 or set errno. Callers must check for errors. If an error
   occurs, the stream is invalid and its resources are freed (and the
   state field set to NULL). The stream may not be used again. It is
   then safe, but not required, to call the corresponding *_end
   function.

   The flags argument to leanodencrypt_init should be 0 by default, or
   leanocrypt_MISMATCH if non-matching keys should be ignored. All other
   values are undefined and reserved for future use.
*/

/* leanodencrypt_multi_init this is a variant of leanodencrypt_init that
   allows a list of n>=1 keys to be defined. During decryption, the
   first matching key is used. This is useful when batch decrypting a
   set of files that have non-uniform keys. If flags includes
   leanocrypt_MISMATCH, then the first key is always used regardless of
   whether it matches or not. */
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
