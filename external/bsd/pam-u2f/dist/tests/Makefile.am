#  Copyright (C) 2014-2018 Yubico AB - See COPYING
#

AUTOMAKE_OPTIONS = serial-tests

AM_CFLAGS = $(WARN_CFLAGS)
AM_CPPFLAGS=-I$(srcdir)/.. -I$(builddir)/..

AM_LDFLAGS = -no-install

LDADD = $(top_builddir)/pam_u2f.la

check_PROGRAMS = basic get_devices

TESTS = $(check_PROGRAMS)
