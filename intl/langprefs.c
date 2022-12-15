
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#if HAVE_CFPREFERENCESCOPYAPPVALUE
# include <string.h>
# include <CFPreferences.h>
# include <CFPropertyList.h>
# include <CFArray.h>
# include <CFString.h>
#endif

const char *
_nl_language_preferences_default (void)
{
#if HAVE_CFPREFERENCESCOPYAPPVALUE /* MacOS X 10.2 or newer */
  {
    static const char *cached_languages;
    static int cache_initialized;

    if (!cache_initialized)
      {
	CFTypeRef preferences =
	  CFPreferencesCopyAppValue (CFSTR ("AppleLanguages"),
				     kCFPreferencesCurrentApplication);
	if (preferences != NULL
	    && CFGetTypeID (preferences) == CFArrayGetTypeID ())
	  {
	    CFArrayRef prefArray = (CFArrayRef)preferences;
	    int n = CFArrayGetCount (prefArray);
	    char buf[256];
	    size_t size = 0;
	    int i;

	    for (i = 0; i < n; i++)
	      {
		CFTypeRef element = CFArrayGetValueAtIndex (prefArray, i);
		if (element != NULL
		    && CFGetTypeID (element) == CFStringGetTypeID ()
		    && CFStringGetCString ((CFStringRef)element,
					   buf, sizeof (buf),
					   kCFStringEncodingASCII))
		  {
		    size += strlen (buf) + 1;
		    /* Most GNU programs use msgids in English and don't ship
		       an en.mo message catalog.  Therefore when we see "en"
		       in the preferences list, arrange for gettext() to
		       return the msgid, and ignore all further elements of
		       the preferences list.  */
		    if (strcmp (buf, "en") == 0)
		      break;
		  }
		else
		  break;
	      }
	    if (size > 0)
	      {
		char *languages = (char *) malloc (size);

		if (languages != NULL)
		  {
		    char *p = languages;

		    for (i = 0; i < n; i++)
		      {
			CFTypeRef element =
			  CFArrayGetValueAtIndex (prefArray, i);
			if (element != NULL
		            && CFGetTypeID (element) == CFStringGetTypeID ()
			    && CFStringGetCString ((CFStringRef)element,
						   buf, sizeof (buf),
						   kCFStringEncodingASCII))
			  {
			    strcpy (p, buf);
			    p += strlen (buf);
			    *p++ = ':';
			    if (strcmp (buf, "en") == 0)
			      break;
			  }
			else
			  break;
		      }
		    *--p = '\0';

		    cached_languages = languages;
		  }
	      }
	  }
	cache_initialized = 1;
      }
    if (cached_languages != NULL)
      return cached_languages;
  }
#endif

  return NULL;
}
