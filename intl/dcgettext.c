#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gettextP.h"
#ifdef _LIBC
# include <libintl.h>
#else
# include "libgnuintl.h"
#endif

/* @@ end of prolog @@ */

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define DCGETTEXT __dcgettext
# define DCIGETTEXT __dcigettext
#else
# define DCGETTEXT libintl_dcgettext
# define DCIGETTEXT libintl_dcigettext
#endif

/* Look up MSGID in the DOMAINNAME message catalog for the current CATEGORY
   locale.  */
char *
DCGETTEXT (const char *domainname, const char *msgid, int category)
{
  return DCIGETTEXT (domainname, msgid, NULL, 0, 0, category);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
INTDEF(__dcgettext)
weak_alias (__dcgettext, dcgettext);
#endif
