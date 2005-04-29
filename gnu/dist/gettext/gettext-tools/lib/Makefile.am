## Makefile for the gettext-tools/lib subdirectory of GNU gettext
## Copyright (C) 1995-1998, 2000-2005 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnits no-dependencies
EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES =
CLEANFILES =
DISTCLEANFILES =
SUFFIXES =

RM = rm -f

lib_LTLIBRARIES = libgettextlib.la

# Sources that are compiled on all platforms.

libgettextlib_la_SOURCES = \
  allocsa.h allocsa.c \
  argmatch.h argmatch.c \
  backupfile.h backupfile.c addext.c \
  basename.h basename.c \
  binary-io.h \
  c-ctype.h c-ctype.c \
  c-strcase.h c-strcasecmp.c c-strncasecmp.c \
  classpath.h classpath.c \
  closeout.h closeout.c \
  copy-file.h copy-file.c \
  csharpcomp.h csharpcomp.c \
  csharpexec.h csharpexec.c \
  error-progname.h error-progname.c \
  execute.h execute.c w32spawn.h \
  exit.h \
  exitfail.h exitfail.c \
  fatal-signal.h fatal-signal.c \
  findprog.h findprog.c \
  fstrcmp.h fstrcmp.c \
  full-write.h full-write.c \
  fwriteerror.h fwriteerror.c \
  gcd.h gcd.c \
  hash.h hash.c \
  javacomp.h javacomp.c \
  javaexec.h javaexec.c \
  linebreak.h linebreak.c lbrkprop.h utf8-ucs4.h utf16-ucs4.h \
  mbswidth.h mbswidth.c \
  minmax.h \
  obstack.h obstack.c \
  pathmax.h \
  pathname.h concatpath.c \
  pipe.h pipe.c w32spawn.h \
  progname.h progname.c progreloc.c \
  quote.h quote.c \
  quotearg.h quotearg.c \
  safe-read.h safe-read.c \
  safe-write.h safe-write.c \
  sh-quote.h sh-quote.c \
  tmpdir.h tmpdir.c \
  ucs4-utf8.h ucs4-utf16.h \
  unlocked-io.h \
  wait-process.h wait-process.c \
  xalloc.h xmalloc.c xstrdup.c \
  xallocsa.h xallocsa.c \
  xerror.h xerror.c \
  xreadlink.h xreadlink.c \
  xsetenv.h xsetenv.c \
  xsize.h

# Sources that are compiled only on platforms that lack the functions.

LIBADD_SOURCE = \
  atexit.c \
  canonicalize.h canonicalize.c \
  error.h error.c \
  fnmatch.c \
  getline.h getline.c \
  getndelim2.h getndelim2.c \
  getopt.c getopt1.c getopt_int.h \
  memmove.c \
  memset.c \
  mkdtemp.h mkdtemp.c \
  readlink.c \
  relocatable.h relocatable.c \
  setenv.h setenv.c unsetenv.c \
  stpcpy.h stpcpy.c \
  stpncpy.h stpncpy.c \
  strcase.h strcasecmp.c strncasecmp.c \
  strcspn.c \
  strerror.c \
  strpbrk.h strpbrk.c \
  strstr.h strstr.c \
  strtol.c \
  strtoul.c \
  vasprintf.h vasprintf.c

# Unused sources.

UNUSED_SOURCE = \
  xgetcwd.h xgetcwd.c

# How to build libgettextlib.la.

libgettextlib_la_LIBADD = @LTLIBOBJS@

# Need @LTLIBINTL@ because many source files use gettext().
# Need @LTLIBICONV@ because linebreak.c uses iconv().
libgettextlib_la_LDFLAGS = \
  -release @VERSION@ \
  @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined

# No need to install libgettextlib.a, except on AIX.
install-exec-local: install-libLTLIBRARIES install-exec-clean
install-exec-clean:
	case "@host_os@" in \
	  aix*) ;; \
	  *) $(RM) $(DESTDIR)$(libdir)/libgettextlib.a ;; \
	esac

# Extra files to be installed.

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = gettext.h

# Files installed for the examples.

docdir = @docdir@
examplesbuildauxdir = $(docdir)/examples/build-aux
examplesbuildaux_DATA = \
  javacomp.sh.in javaexec.sh.in \
  csharpcomp.sh.in csharpexec.sh.in

# List of files to be distributed.

noinst_HEADERS = gettext.h

EXTRA_DIST += \
  $(LIBADD_SOURCE) $(UNUSED_SOURCE) relocwrapper.c allocsa.valgrind \
  gen-lbrkprop.c 3level.h Combining.txt \
  ChangeLog.0

AM_CPPFLAGS = -I. -I$(srcdir) -I.. -I../intl -I$(top_srcdir)/intl

DEFS = -DEXEEXT=\"$(EXEEXT)\" -DDEPENDS_ON_LIBINTL=1 -DDEPENDS_ON_LIBICONV=1 @DEFS@


# >>> gnulib module stdbool.
EXTRA_DIST += stdbool_.h

# The following is needed in order to create an <stdbool.h> when the system
# doesn't have one that works.
all-local $(libgettextlib_la_OBJECTS): @STDBOOL_H@
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/@HAVE__BOOL@/g' < $(srcdir)/stdbool_.h > stdbool.h
MOSTLYCLEANFILES += stdbool.h
# <<< gnulib module stdbool.


# >>> gnulib module alloca.
EXTRA_DIST += alloca_.h

# The following is needed in order to create an <alloca.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libgettextlib_la_OBJECTS): @ALLOCA_H@
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h alloca.h
MOSTLYCLEANFILES += alloca.h
# <<< gnulib module alloca.


# >>> gnulib module fnmatch.
BUILT_SOURCES += $(FNMATCH_H)
EXTRA_DIST += fnmatch_.h fnmatch_loop.c

# The following is needed in order to create an <fnmatch.h> when the system
# doesn't have one that works.
all-local $(libgettextlib_la_OBJECTS): @FNMATCH_H@
fnmatch.h: fnmatch_.h
	cp $(srcdir)/fnmatch_.h fnmatch.h
MOSTLYCLEANFILES += fnmatch.h
# <<< gnulib module fnmatch.


# >>> gnulib module getopt.
BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

# The following is needed in order to create a <getopt.h> when the system
# doesn't have one that works.
all-local $(libgettextlib_la_OBJECTS): @GETOPT_H@
getopt.h: getopt_.h
	cp $(srcdir)/getopt_.h getopt.h
MOSTLYCLEANFILES += getopt.h
# <<< gnulib module getopt.


# >>> gnulib module localcharset.
libgettextlib_la_SOURCES += localcharset.h localcharset.c
EXTRA_DIST += config.charset ref-add.sin ref-del.sin
DEFS += -DLIBDIR=\"$(libdir)\"

# The following is needed in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

all-local: charset.alias ref-add.sed ref-del.sed

charset_alias = $(DESTDIR)$(libdir)/charset.alias
charset_tmp = $(DESTDIR)$(libdir)/charset.tmp
install-exec-local: all-local
	test @GLIBC21@ != no || $(mkinstalldirs) $(DESTDIR)$(libdir)
	if test -f $(charset_alias); then \
	  sed -f ref-add.sed $(charset_alias) > $(charset_tmp) ; \
	  $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	  rm -f $(charset_tmp) ; \
	else \
	  if test @GLIBC21@ = no; then \
	    sed -f ref-add.sed charset.alias > $(charset_tmp) ; \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	    rm -f $(charset_tmp) ; \
	  fi ; \
	fi

uninstall-local: all-local
	if test -f $(charset_alias); then \
	  sed -f ref-del.sed $(charset_alias) > $(charset_tmp); \
	  if grep '^# Packages using this file: $$' $(charset_tmp) \
	      > /dev/null; then \
	    rm -f $(charset_alias); \
	  else \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias); \
	  fi; \
	  rm -f $(charset_tmp); \
	fi

charset.alias: config.charset
	$(SHELL) $(srcdir)/config.charset '@host@' > t-$@
	mv t-$@ $@

SUFFIXES += .sed .sin
.sin.sed:
	sed -e '/^#/d' -e 's/@''PACKAGE''@/@PACKAGE@/g' $< > t-$@
	mv t-$@ $@

CLEANFILES += charset.alias ref-add.sed ref-del.sed
# <<< gnulib module localcharset.


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
