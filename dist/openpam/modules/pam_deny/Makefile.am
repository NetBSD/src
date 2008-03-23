# $Id: Makefile.am,v 1.1.1.2.10.1 2008/03/23 00:17:07 matt Exp $

pkglibdir = @OPENPAM_MODULES_DIR@
INCLUDES = -I$(top_srcdir)/include

pkglib_LTLIBRARIES = pam_deny.la

pam_deny_la_SOURCES = pam_deny.c
pam_deny_la_LDFLAGS = -no-undefined -module -version-info @LIB_MAJ@
pam_deny_la_LIBADD = $(top_builddir)/lib/libpam.la
