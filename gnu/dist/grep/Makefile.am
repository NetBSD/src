## Process this file with automake to create Makefile.in
AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = intl po doc src tests djgpp m4 vms bootstrap

EXTRA_DIST = TODO README PATCHES.AC PATCHES.AM

# We should be able to just define `ACLOCAL_AMFLAGS = --acdir=m4',
# but this runs afoul of a bug in automake 1.
# The following hack works around this bug by creating acinclude.m4 manually.
ACLOCAL_AMFLAGS =
M4DIR = $(srcdir)/m4
ACINCLUDE_INPUTS = $(M4DIR)/djgpp.m4 $(M4DIR)/dosfile.m4 \
  $(M4DIR)/envsep.m4 $(M4DIR)/gettext.m4 $(M4DIR)/glibc.m4 \
  $(M4DIR)/header.m4 $(M4DIR)/init.m4 $(M4DIR)/install.m4 \
  $(M4DIR)/isc-posix.m4 $(M4DIR)/largefile.m4 $(M4DIR)/lcmessage.m4 \
  $(M4DIR)/missing.m4 $(M4DIR)/progtest.m4 $(M4DIR)/regex.m4 \
  $(M4DIR)/sanity.m4
$(srcdir)/acinclude.m4 : $(ACINCLUDE_INPUTS)
	cat $(ACINCLUDE_INPUTS) >$(srcdir)/acinclude.m4
