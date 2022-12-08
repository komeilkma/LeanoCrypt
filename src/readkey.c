/* Copyright (C) 2022 Komeil Majidi.*/
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xalloc.h"

#include "gettext.h"
#define _(String) gettext (String)

/* read key from /dev/tty. If echo is 0, hide the typed characters. */
char *readkey(const char *prompt, const char *promptcont, const char *myname, int echo) {
  char *line;
  FILE *fin;
  struct termios tio, saved_tio;

  fin = fopen("/dev/tty", "r");
  if (fin==NULL) {
    fprintf(stderr, _("%s: cannot open /dev/tty: %s\n"), myname, strerror(errno));
    exit(2);
  }

  fprintf(stderr, "%s%s", prompt, promptcont);
  fflush(stderr);

  if (!echo) {
    /* disable echo */
    tcgetattr(fileno(fin), &tio);
    saved_tio = tio;
    tio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(fileno(fin), TCSANOW, &tio);
  }

  /* read key */
  line = xreadline(fin, myname);

  if (!echo) {
    /* restore echo */
    tcsetattr(fileno(fin), TCSANOW, &saved_tio);
  }
  /* print newline, close file */
  fprintf(stderr, "\n");
  fflush(stderr);
  fclose(fin);

  return line;
}
