#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Print an ASCII string with quotes and escape sequences where needed.  */
static void
print_escaped (FILE *stream, const char *str)
{
  putc ('"', stream);
  for (; *str != '\0'; str++)
    if (*str == '\n')
      {
	fputs ("\\n\"", stream);
	if (str[1] == '\0')
	  return;
	fputs ("\n\"", stream);
      }
    else
      {
	if (*str == '"' || *str == '\\')
	  putc ('\\', stream);
	putc (*str, stream);
      }
  putc ('"', stream);
}

/* Add to the log file an entry denoting a failed translation.  */
void
_nl_log_untranslated (const char *logfilename, const char *domainname,
		      const char *msgid1, const char *msgid2, int plural)
{
  static char *last_logfilename = NULL;
  static FILE *last_logfile = NULL;
  FILE *logfile;

  /* Can we reuse the last opened logfile?  */
  if (last_logfilename == NULL || strcmp (logfilename, last_logfilename) != 0)
    {
      /* Close the last used logfile.  */
      if (last_logfilename != NULL)
	{
	  if (last_logfile != NULL)
	    {
	      fclose (last_logfile);
	      last_logfile = NULL;
	    }
	  free (last_logfilename);
	  last_logfilename = NULL;
	}
      /* Open the logfile.  */
      last_logfilename = (char *) malloc (strlen (logfilename) + 1);
      if (last_logfilename == NULL)
	return;
      strcpy (last_logfilename, logfilename);
      last_logfile = fopen (logfilename, "a");
      if (last_logfile == NULL)
	return;
    }
  logfile = last_logfile;

  fprintf (logfile, "domain ");
  print_escaped (logfile, domainname);
  fprintf (logfile, "\nmsgid ");
  print_escaped (logfile, msgid1);
  if (plural)
    {
      fprintf (logfile, "\nmsgid_plural ");
      print_escaped (logfile, msgid2);
      fprintf (logfile, "\nmsgstr[0] \"\"\n");
    }
  else
    fprintf (logfile, "\nmsgstr \"\"\n");
  putc ('\n', logfile);
}
