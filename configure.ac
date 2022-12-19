dnl  Copyright (C) 2022 Komeil Majidi.
dnl  This file is part of leanocrypt. It is free software and it is covered
dnl  by the GNU general public license. See the file COPYING for details.

dnl  Process this file with autoconf to produce a configure script.

dnl ----------------------------------------------------------------------
AC_INIT
AC_CONFIG_SRCDIR([src/leanocrypt.c])
AM_INIT_AUTOMAKE(leanocrypt, 1.10)
DATE="October 2012"
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl ----------------------------------------------------------------------
dnl  Find lisp installation directory
AM_PATH_LISPDIR

dnl ----------------------------------------------------------------------
dnl  The names of the installed executables and the default suffix are in
dnl  principle configurable. However, they should not normally be changed,
dnl  because other scripts might depend on them.

NAMEleanocrypt=leanocrypt
NAMEENCRYPT=leanoencrypt
NAMEDECRYPT=leanodencrypt
NAMECAT=ccat
SUF=.leanocrypt

dnl Upper case name
NAMEUC=`echo $NAMEleanocrypt | tr a-z A-Z`

dnl ----------------------------------------------------------------------
dnl Export some parameters to config file
AC_DEFINE_UNQUOTED(NAMEleanocrypt,"$NAMEleanocrypt",Name of the leanocrypt binary)
AC_DEFINE_UNQUOTED(NAMEENCRYPT,"$NAMEENCRYPT",Name of the leanoencrypt binary)
AC_DEFINE_UNQUOTED(NAMEDECRYPT,"$NAMEDECRYPT",Name of the leanodencrypt binary)
AC_DEFINE_UNQUOTED(NAMECAT,"$NAMECAT",Name of the ccat binary)
AC_DEFINE_UNQUOTED(SUF,"$SUF",Default suffix for encrypted files)

dnl ----------------------------------------------------------------------
dnl remember user's CFLAGS
iCFLAGS="$CFLAGS"

dnl ----------------------------------------------------------------------
dnl Check for C compiler and flags
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC

dnl If compiler is gcc, add compiler warnings unless user overrides CFLAGS
if test "$GCC" = "yes" && test "$iCFLAGS" = ""; then
   CFLAGS="$CFLAGS -Wall"
fi

dnl Also add CADD to the CFLAGS at configure time or compile time
AC_SUBST(CADD)

AC_MSG_CHECKING(what compiler options to use)
AC_MSG_RESULT($CFLAGS $CADD)

dnl ----------------------------------------------------------------------
dnl Check for other programs

AC_CHECK_PROGS(TAR, gtar, tar)

dnl ----------------------------------------------------------------------
dnl enable large file support

AC_SYS_LARGEFILE

dnl ----------------------------------------------------------------------
dnl check for features

AC_ARG_ENABLE(libcrypt,
 AS_HELP_STRING([--disable-libcrypt],[do not link against libcrypt, use own replacement]))

AC_ARG_ENABLE(emacs,
 AS_HELP_STRING([--disable-emacs],[omit emacs support]),
 if test "$enableval" = no; then
     EMACS=no
 fi   
)

dnl ----------------------------------------------------------------------
dnl Checks for libraries.

dnl Unless explicitly disabled, link against libcrypt if possible
if test "$enable_libcrypt" != no; then
  AC_CHECK_LIB(crypt, crypt)
fi

dnl If not linking against libcrypt, must link against replacement
if test "$ac_cv_lib_crypt_crypt" != yes; then
   EXTRA_OBJS="$EXTRA_OBJS unixcrypt3.o"
fi

dnl SCO Open Server requires -lsocket for gethostname()
AC_CHECK_LIB(socket, gethostname)

dnl ----------------------------------------------------------------------
dnl Checks for header files.
AC_CHECK_HEADERS(stdint.h crypt.h)

dnl ----------------------------------------------------------------------
dnl Checks for library functions.
AC_CHECK_FUNC(getopt_long, , EXTRA_OBJS="$EXTRA_OBJS getopt.o getopt1.o")

dnl ----------------------------------------------------------------------
dnl Find sizes of some types
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)

dnl Determine 32-bit unsigned integer type
AC_MSG_CHECKING([for 32 bit unsigned integer type])
if test "$ac_cv_sizeof_unsigned_int" -eq 4; then
   UINT32_TYPE="unsigned int";
elif test "$ac_cv_sizeof_unsigned_long" -eq 4; then
   UINT32_TYPE="unsigned long";
else
AC_MSG_ERROR(cannot find 32 bit integer type)
fi
AC_MSG_RESULT($UINT32_TYPE)
AC_DEFINE_UNQUOTED(UINT32_TYPE,$UINT32_TYPE,unsigned 32 bit integer type)

dnl ----------------------------------------------------------------------
dnl Internationalization

GETTEXT_PACKAGE=leanocrypt
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])

AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.14.3])
IT_PO_SUBDIR(po)

dnl ----------------------------------------------------------------------
dnl Libtool (needed by intl/)

LT_INIT

dnl ----------------------------------------------------------------------
dnl Set up substitutions of non-standard configuration parameters
AC_SUBST(NAMEleanocrypt)
AC_SUBST(NAMEENCRYPT)
AC_SUBST(NAMEDECRYPT)
AC_SUBST(NAMECAT)
AC_SUBST(SUF)
AC_SUBST(DATE)
AC_SUBST(NAMEUC)
AC_SUBST(EXTRA_OBJS)
AC_SUBST(TAR)

dnl ----------------------------------------------------------------------
AC_CONFIG_FILES([m4/Makefile 
	   intl/Makefile 
	   Makefile 
	   src/Makefile 
          ])
AC_OUTPUT
