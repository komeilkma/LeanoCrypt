
#ifndef _VASNPRINTF_H
#define _VASNPRINTF_H

/* Get va_list.  */
#include <stdarg.h>

/* Get size_t.  */
#include <stddef.h>

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern char * asnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)));
extern char * vasnprintf (char *resultbuf, size_t *lengthp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 3, 0)));

#ifdef	__cplusplus
}
#endif

#endif /* _VASNPRINTF_H */
