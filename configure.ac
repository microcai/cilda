# Autoconf file for tilda
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([Cilda], [0.9.7], [])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/cilda.c])
AM_CONFIG_HEADER([cilda-config.h:cilda-config.h.in])

if test "x$enable_maintainer_flags" = "xyes"; then
	enable_debug="yes"
	enable_debug_functions="yes"
	CFLAGS="$CFLAGS -Wall -Wextra -pedantic -fno-common -Wstrict-aliasing=2"
	CFLAGS="$CFLAGS -Wshadow -Wmissing-prototypes -Wmissing-declarations"
fi

AC_ARG_ENABLE([debug-functions], [AC_HELP_STRING([--enable-debug-functions], [enable function call tracing])])

if test "x$enable_debug_functions" = "xyes"; then
	AC_DEFINE([DEBUG_FUNCTIONS], [1], [Enable function call tracing])
fi

# Checks for programs.
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_PROG_LEX

# Notice if there are no versions of lex installed
# and error out of the configure.
case $LEX in
	:)
		AC_MSG_ERROR([No lex program found])
		;;
esac

# Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32 ])
PKG_CHECK_MODULES([VTE], [vte-2.90])
PKG_CHECK_MODULES([LIBCONFUSE], [libconfuse])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 cilda.desktop.in ])
AC_OUTPUT
