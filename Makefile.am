## Process this file with automake to produce to Makefile.in
# vim: set ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

Applicationsdir = ${datadir}/applications
Applications_DATA = cilda.desktop

Pixmapsdir = ${datadir}/pixmaps
Pixmaps_DATA = cilda.png

data_DATA = cilda.ui

EXTRA_DIST = m4 cilda.desktop.in.in cilda.png cilda.ui


cilda.desktop.in:cilda.desktop.in.in
	./config.status $@

cilda.desktop:cilda.desktop.in
	( \
		s=`cat $<` ; \
		eval "echo \"$${s}\"";\
	) > $@
