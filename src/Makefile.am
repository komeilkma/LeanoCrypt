## Copyright (C) 2022 Komeil Majidi.
AM_CFLAGS = $(CADD)

# programs
bin_PROGRAMS = leanocrypt ccguess

leanocrypt_SOURCES = main.c main.h traverse.c traverse.h xalloc.c xalloc.h	\
  readkey.c readkey.h leanocrypt.c leanocrypt.h unixcryptlib.c unixcryptlib.h	\
  gettext.h platform.h platform.c
leanocrypt_LDADD = @EXTRA_OBJS@ libleanocrypt.a
leanocrypt_DEPENDENCIES = @EXTRA_OBJS@ libleanocrypt.a

ccguess_SOURCES = ccguess.c rijndael.h rijndael.c tables.h tables.c lists.h readkey.c readkey.h xalloc.c xalloc.h

# libraries
noinst_LIBRARIES = libleanocrypt.a
libleanocrypt_a_SOURCES = leanocryptlib.c leanocryptlib.h rijndael.h rijndael.c tables.h tables.c platform.h

# stuff that automake can't figure out on its own
EXTRA_DIST = getopt.c getopt1.c getopt.h unixcrypt3.c unixcrypt3.h maketables.c
MOSTLYCLEANFILES = maketables
MAINTAINERCLEANFILES = tables.c

maketables: maketables.c

tables.c: maketables.c
	$(MAKE) maketables
	./maketables > tables.c

# install/uninstall symbolic links
install-exec-hook:
	test "@NAMEleanocrypt@" = leanocrypt || \
	mv -f $(DESTDIR)$(bindir)/leanocrypt $(DESTDIR)$(bindir)/@NAMEleanocrypt@
	ln -f -s @NAMEleanocrypt@ $(DESTDIR)$(bindir)/@NAMEENCRYPT@
	ln -f -s @NAMEleanocrypt@ $(DESTDIR)$(bindir)/@NAMEDECRYPT@
	ln -f -s @NAMEleanocrypt@ $(DESTDIR)$(bindir)/@NAMECAT@

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/@NAMEleanocrypt@
	rm -f $(DESTDIR)$(bindir)/@NAMEENCRYPT@
	rm -f $(DESTDIR)$(bindir)/@NAMEDECRYPT@
	rm -f $(DESTDIR)$(bindir)/@NAMECAT@

# internationalization stuff
localedir = $(datadir)/locale
INCLUDES = -I../intl -I$(top_srcdir)/intl -DLOCALEDIR=\"$(localedir)\"
LIBS = @LIBINTL@ @LIBS@
