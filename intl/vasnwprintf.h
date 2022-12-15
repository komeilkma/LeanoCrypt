
#ifndef _VASNWPRINTF_H
#define _VASNWPRINTF_H

/* Get va_list.  */
#include <stdarg.h>

/* Get wchar_t, size_t.  */
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern wchar_t * asnwprintf (wchar_t *resultbuf, size_t *lengthp, const wchar_t *format, ...);
extern wchar_t * vasnwprintf (wchar_t *resultbuf, size_t *lengthp, const wchar_t *format, va_list args);

#ifdef	__cplusplus
}
#endif

#endif /* _VASNWPRINTF_H */
