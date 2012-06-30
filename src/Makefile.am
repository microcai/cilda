## Process this file with automake to produce Makefile.in
# vim: set ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

# Define a C macro LOCALEDIR indicating where catalogs 
# will be installed.
#localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DDATADIR=\"$(datadir)\" @DEFS@

bin_PROGRAMS = tilda

# Keep the headers here so that make dist-bzip2 works
tilda_SOURCES = configsys.h configsys.c \
		debug.h \
		eggaccelerators.h eggaccelerators.c \
		gettext.h \
		key_converter.h key_converter.l \
		key_grabber.h key_grabber.c \
		tilda.h tilda.c \
		tilda_terminal.h tilda_terminal.c \
		tilda_window.h tilda_window.c \
		tomboykeybinder.h tomboykeybinder.c \
		translation.h \
		wizard.h wizard.c 

tilda_CFLAGS = $(AM_CFLAGS) \
		@GTK_CFLAGS@ \
		@GLIB_CFLAGS@ \
		@VTE_CFLAGS@ \
		@LIBCONFUSE_CFLAGS@

tilda_LDADD = $(AM_LDADD) \
		@GTK_LIBS@ \
		@GLIB_LIBS@ \
		@VTE_LIBS@ \
		@LIBCONFUSE_LIBS@ \
		@LTLIBINTL@ \
		@LEXLIB@
