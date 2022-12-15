
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gettextP.h"
#undef gettext
#undef dgettext
#undef dcgettext
#undef ngettext
#undef dngettext
#undef dcngettext
#undef textdomain
#undef bindtextdomain
#undef bind_textdomain_codeset


/* When building a DLL, we must export some functions.  Note that because
   the functions are only defined for binary backward compatibility, we
   don't need to use __declspec(dllimport) in any case.  */
#if defined _MSC_VER && BUILDING_DLL
# define DLL_EXPORTED __declspec(dllexport)
#else
# define DLL_EXPORTED
#endif


DLL_EXPORTED
char *
gettext (const char *msgid)
{
  return libintl_gettext (msgid);
}


DLL_EXPORTED
char *
dgettext (const char *domainname, const char *msgid)
{
  return libintl_dgettext (domainname, msgid);
}


DLL_EXPORTED
char *
dcgettext (const char *domainname, const char *msgid, int category)
{
  return libintl_dcgettext (domainname, msgid, category);
}


DLL_EXPORTED
char *
ngettext (const char *msgid1, const char *msgid2, unsigned long int n)
{
  return libintl_ngettext (msgid1, msgid2, n);
}


DLL_EXPORTED
char *
dngettext (const char *domainname,
	   const char *msgid1, const char *msgid2, unsigned long int n)
{
  return libintl_dngettext (domainname, msgid1, msgid2, n);
}


DLL_EXPORTED
char *
dcngettext (const char *domainname,
	    const char *msgid1, const char *msgid2, unsigned long int n,
	    int category)
{
  return libintl_dcngettext (domainname, msgid1, msgid2, n, category);
}


DLL_EXPORTED
char *
textdomain (const char *domainname)
{
  return libintl_textdomain (domainname);
}


DLL_EXPORTED
char *
bindtextdomain (const char *domainname, const char *dirname)
{
  return libintl_bindtextdomain (domainname, dirname);
}


DLL_EXPORTED
char *
bind_textdomain_codeset (const char *domainname, const char *codeset)
{
  return libintl_bind_textdomain_codeset (domainname, codeset);
}
