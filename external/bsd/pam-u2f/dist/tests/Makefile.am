#  Copyright (C) 2014-2018 Yubico AB - See COPYING
#

AM_CFLAGS = $(WARN_CFLAGS)
AM_CPPFLAGS=-I$(srcdir)/.. -I$(builddir)/..

AM_LDFLAGS = -no-install

LDADD = $(top_builddir)/pam_u2f.la

check_PROGRAMS = basic
TESTS = $(check_PROGRAMS)
