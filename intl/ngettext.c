
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _LIBC
# define __need_NULL
# include <stddef.h>
#else
# include <stdlib.h>		/* Just for NULL.  */
#endif

#include "gettextP.h"
#ifdef _LIBC
# include <libintl.h>
#else
# include "libgnuintl.h"
#endif

#include <locale.h>


#ifdef _LIBC
# define NGETTEXT __ngettext
# define DCNGETTEXT __dcngettext
#else
# define NGETTEXT libintl_ngettext
# define DCNGETTEXT libintl_dcngettext
#endif

char *
NGETTEXT (const char *msgid1, const char *msgid2, unsigned long int n)
{
  return DCNGETTEXT (NULL, msgid1, msgid2, n, LC_MESSAGES);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
weak_alias (__ngettext, ngettext);
#endif
