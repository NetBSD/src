## Process this file with automake to produce Makefile.in
# Makefile for library files used by GNU CVS.
#
# Copyright (C) 1986-2005 The Free Software Foundation, Inc.
#
# Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
#                                  and others.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

noinst_LIBRARIES = libcvs.a

# Should look into unifying regular expression matching in CVS
# with the diff library (perhaps to have the caller, CVS, do the
# matching?)
libcvs_a_SOURCES = \
	sighandle.c \
	system.h \
	wait.h \
	xselect.h \
	xtime.h
## Because @LIBOBJS@ is included below and AC_LIBOBJ is sometimes called on
## these files from configure.in, automake automatically knows about many of
## the *.c files in this directory.
libcvs_a_LIBADD = \
	@LIBOBJS@



## Zero some variables so Automake will allow us to use += on them.
BUILT_SOURCES =
MOSTLYCLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
TESTS =
check_PROGRAMS =

EXTRA_DIST = \
	.cvsignore \
	ChangeLog.fsf \
	Makefile.gnulib \
	build_lib.com \
	libcvs.dep \
	libcvs.dsp \
	libcvs.mak \
	test-getdate.sh

## begin gnulib module alloca


libcvs_a_LIBADD += @ALLOCA@
## end   gnulib module alloca

## begin gnulib module alloca-opt

BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create <alloca.h> when the system
# doesn't have one that works with the given compiler.
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

## end   gnulib module alloca-opt

## begin gnulib module allocsa

libcvs_a_SOURCES += allocsa.h allocsa.c
EXTRA_DIST += allocsa.valgrind

## end   gnulib module allocsa

## begin gnulib module cycle-check

libcvs_a_SOURCES += cycle-check.c cycle-check.h dev-ino.h

## end   gnulib module cycle-check

## begin gnulib module dirname

libcvs_a_SOURCES += basename.c stripslash.c

## end   gnulib module dirname

## For GNULIB's error module.
##
## This module isn't fully imported since GNULIB's error.c would conflict with
## src/error.c (which knows how to send error messages over the network).  We
## don't compile error.c into libcvs on purpose.  It can get compiled later as
## part of the getdate test program,
libcvs_a_SOURCES += error.h

## begin gnulib module exit

libcvs_a_SOURCES += exit.h

## end   gnulib module exit

## begin gnulib module fnmatch

BUILT_SOURCES += $(FNMATCH_H)
EXTRA_DIST += fnmatch_.h fnmatch_loop.c

# We need the following in order to create <fnmatch.h> when the system
# doesn't have one that supports the required API.
fnmatch.h: fnmatch_.h
	cp $(srcdir)/fnmatch_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += fnmatch.h fnmatch.h-t

## end   gnulib module fnmatch

## begin gnulib module getaddrinfo

libcvs_a_SOURCES += getaddrinfo.h

## end   gnulib module getaddrinfo

## begin gnulib module getdate

## CVS test scripts for getdate.
TESTS += test-getdate.sh
MOSTLYCLEANFILES += getdate-expected getdate-got getdate.diff
DISTCLEANFILES += getdate.log
# Program required by test-getdate.sh for testing getdate.y.
check_PROGRAMS += getdate
getdate_SOURCES = \
	error.c \
	getdate.y
## This source file was added only for the getdate test program when compiled
## with GNULIB's error.c.
getdate_SOURCES += \
	progname.c
getdate_CPPFLAGS = -DTEST
getdate_LDADD = \
	$(noinst_LIBRARIES) \
	$(LIB_CLOCK_GETTIME) \
	$(LIBINTL)

BUILT_SOURCES += getdate.c
MAINTAINERCLEANFILES += getdate.c
EXTRA_DIST += getdate.c

## end   gnulib module getdate

## begin gnulib module getndelim2

EXTRA_DIST += getndelim2.h getndelim2.c

## end   gnulib module getndelim2

## begin gnulib module getnline

libcvs_a_SOURCES += getnline.h getnline.c

## end   gnulib module getnline

## begin gnulib module getopt

BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

# We need the following in order to create <getopt.h> when the system
# doesn't have one that works with the given compiler.
getopt.h: getopt_.h
	cp $(srcdir)/getopt_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += getopt.h getopt.h-t

## end   gnulib module getopt

## begin gnulib module gettext-h

libcvs_a_SOURCES += gettext.h

## end   gnulib module gettext-h

## begin gnulib module glob

BUILT_SOURCES += $(GLOB_H)

# We need the following in order to create an <getopt.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libcvs_a_OBJECTS): $(GLOB_H)
glob.h: glob_.h
	cp $(srcdir)/glob_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += glob.h glob.h-t

## end   gnulib module glob.c

## begin gnulib module mbuiter

libcvs_a_SOURCES += mbuiter.h

## end   gnulib module mbuiter

## begin gnulib module minmax

libcvs_a_SOURCES += minmax.h

## end   gnulib module minmax

## begin gnulib module setenv

libcvs_a_SOURCES += setenv.h

## end   gnulib module setenv

## begin gnulib module size_max

libcvs_a_SOURCES += size_max.h

## end   gnulib module size_max

## begin gnulib module stdbool

BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h

# We need the following in order to create <stdbool.h> when the system
# doesn't have one that works.
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' \
		< $(srcdir)/stdbool_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

## end   gnulib module stdbool

## begin gnulib module stdint

BUILT_SOURCES += $(STDINT_H)
EXTRA_DIST += stdint_.h

# We need the following in order to create <stdint.h> when the system
# doesn't have one that works with the given compiler.
stdint.h: stdint_.h
	sed -e 's/@''HAVE_LONG_64BIT''@/$(HAVE_LONG_64BIT)/g;s/@''HAVE_LONG_LONG_64BIT@/$(HAVE_LONG_LONG_64BIT)/g' < $(srcdir)/stdint_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdint.h stdint.h-t

## end   gnulib module stdint

## begin gnulib module strcase

libcvs_a_SOURCES += strcase.h

## end   gnulib module strcase

## begin gnulib module strnlen1

libcvs_a_SOURCES += strnlen1.h strnlen1.c

## end   gnulib module strnlen1

## begin gnulib module strstr

libcvs_a_SOURCES += strstr.h

## end   gnulib module strstr

## begin gnulib module time_r

libcvs_a_SOURCES += time_r.h

## end   gnulib module time_r

## begin gnulib module vasnprintf

libcvs_a_SOURCES += printf-args.h printf-parse.h vasnprintf.h

## end   gnulib module vasnprintf

## begin gnulib module vasprintf

libcvs_a_SOURCES += vasprintf.h

## end   gnulib module vasprintf

## begin gnulib module xalloc-die

libcvs_a_SOURCES += xalloc-die.c

## end   gnulib module xalloc-die

## begin gnulib module xgethostname

libcvs_a_SOURCES += xgethostname.h xgethostname.c

## end   gnulib module xgethostname

## begin gnulib module xreadlink

libcvs_a_SOURCES += xreadlink.h xreadlink.c

## end   gnulib module xreadlink

## begin gnulib module xsize

libcvs_a_SOURCES += xsize.h

## end   gnulib module xsize

# Until Automake gets its act together
distclean-local:
	rm -f fnmatch.h

# for backwards compatibility with the old makefiles
realclean: maintainer-clean
.PHONY: realclean
