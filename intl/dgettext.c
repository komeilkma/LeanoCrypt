
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gettextP.h"

#include <locale.h>

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
# define DGETTEXT __dgettext
# define DCGETTEXT INTUSE(__dcgettext)
#else
# define DGETTEXT libintl_dgettext
# define DCGETTEXT libintl_dcgettext
#endif

/* Look up MSGID in the DOMAINNAME message catalog of the current
   LC_MESSAGES locale.  */
char *
DGETTEXT (const char *domainname, const char *msgid)
{
  return DCGETTEXT (domainname, msgid, LC_MESSAGES);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
weak_alias (__dgettext, dgettext);
#endif
