
#ifndef _PRINTF_PARSE_H
#define _PRINTF_PARSE_H

#include "printf-args.h"


/* Flags */
#define FLAG_GROUP	 1	/* ' flag */
#define FLAG_LEFT	 2	/* - flag */
#define FLAG_SHOWSIGN	 4	/* + flag */
#define FLAG_SPACE	 8	/* space flag */
#define FLAG_ALT	16	/* # flag */
#define FLAG_ZERO	32

/* arg_index value indicating that no argument is consumed.  */
#define ARG_NONE	(~(size_t)0)

/* A parsed directive.  */
typedef struct
{
  const char* dir_start;
  const char* dir_end;
  int flags;
  const char* width_start;
  const char* width_end;
  size_t width_arg_index;
  const char* precision_start;
  const char* precision_end;
  size_t precision_arg_index;
  char conversion; /* d i o u x X f e E g G c s p n U % but not C S */
  size_t arg_index;
}
char_directive;

/* A parsed format string.  */
typedef struct
{
  size_t count;
  char_directive *dir;
  size_t max_width_length;
  size_t max_precision_length;
}
char_directives;

#ifdef STATIC
STATIC
#else
extern
#endif
int printf_parse (const char *format, char_directives *d, arguments *a);

#endif /* _PRINTF_PARSE_H */
