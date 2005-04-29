## Makefile for the gettext-runtime/libasprintf subdirectory of GNU gettext
## Copyright (C) 2002-2005 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify it
## under the terms of the GNU Library General Public License as published
## by the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Library General Public License for more details.
##
## You should have received a copy of the GNU Library General Public
## License along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
## USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I ../../m4 -I ../m4
EXTRA_DIST =
MOSTLYCLEANFILES =

RM = rm -f


# Library include file.

include_HEADERS = autosprintf.h

all-local $(libasprintf_la_OBJECTS): autosprintf.h
autosprintf.h: autosprintf.h.in
	cp $(srcdir)/autosprintf.h.in autosprintf.h
MOSTLYCLEANFILES += autosprintf.h
EXTRA_DIST += autosprintf.h.in

dist-hook:
	rm -f $(distdir)/autosprintf.h


# Library code.

lib_LTLIBRARIES = libasprintf.la

libasprintf_la_SOURCES = \
  lib-asprintf.h lib-asprintf.c \
  autosprintf.h autosprintf.cc

# Sources used only on platforms lacking vasprintf().
lib_asprintf_EXTRASOURCES = \
  xsize.h \
  printf-args.h printf-args.c \
  printf-parse.h printf-parse.c \
  vasnprintf.h vasnprintf.c asnprintf.c \
  vasprintf.h vasprintf.c asprintf.c
lib-asprintf.lo: $(lib_asprintf_EXTRASOURCES)
EXTRA_DIST += $(lib_asprintf_EXTRASOURCES)


# How to build libasprintf.
# With libtool 1.5.14, on some platforms, like BeOS, "libtool --tag=CXX" fails
# to create a shared library, however "libtool --tag=CC" succeeds.
libasprintf.la: $(libasprintf_la_OBJECTS) $(libasprintf_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(libasprintf_la_LDFLAGS) $(libasprintf_la_OBJECTS) $(libasprintf_la_LIBADD) $(LIBS) || \
	$(LINK) -rpath $(libdir) $(libasprintf_la_LDFLAGS) $(libasprintf_la_OBJECTS) $(libasprintf_la_LIBADD) $(LIBS)


# >>> gnulib module alloca.
EXTRA_DIST += alloca_.h

# The following is needed in order to create an <alloca.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libasprintf_la_OBJECTS): @ALLOCA_H@
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h alloca.h
MOSTLYCLEANFILES += alloca.h
# <<< gnulib module alloca.


# Clean up after Solaris cc.
clean-local:
	rm -rf SunWS_cache


# Documentation.

docdir = $(datadir)/doc/@PACKAGE@
dvidir = $(docdir)
psdir = $(docdir)
pdfdir = $(docdir)
htmldir = $(docdir)

MAKEINFO = env LANG= LC_MESSAGES= LC_ALL= LANGUAGE= @MAKEINFO@
MAKEINFOFLAGS = --no-split

info_TEXINFOS = autosprintf.texi

# We distribute only the HTML documentation.
# The user can generate the others, via
#   make autosprintf.ps
#   make autosprintf.pdf

all-local: html-local
install-data-local: install-html
installdirs-local: installdirs-html
uninstall-local: uninstall-html

CLEANFILES = autosprintf.pdf
# autosprintf.dvi and autosprintf.ps are already known to automake.


# Documentation in DVI format.

install-dvi: autosprintf.dvi
	$(mkinstalldirs) $(DESTDIR)$(dvidir)
	$(INSTALL_DATA) `if test -f autosprintf.dvi; then echo .; else echo $(srcdir); fi`/autosprintf.dvi $(DESTDIR)$(dvidir)/autosprintf.dvi

installdirs-dvi:
	$(mkinstalldirs) $(DESTDIR)$(dvidir)

uninstall-dvi:
	$(RM) $(DESTDIR)$(dvidir)/autosprintf.dvi

# Temporary index files. automake removes only the predefined ones by itself.
MOSTLYCLEANFILES +=


# Documentation in Postscript format.

# Override of automake's definition:
#DVIPS = @DVIPS@
DVIPS = @DVIPS@ -D600

autosprintf.ps: autosprintf.dvi
	$(DVIPS) -o $@ `if test -f autosprintf.dvi; then echo autosprintf.dvi; else echo $(srcdir)/autosprintf.dvi; fi`

install-ps: autosprintf.ps
	$(mkinstalldirs) $(DESTDIR)$(psdir)
	$(INSTALL_DATA) `if test -f autosprintf.ps; then echo .; else echo $(srcdir); fi`/autosprintf.ps $(DESTDIR)$(psdir)/autosprintf.ps

installdirs-ps:
	$(mkinstalldirs) $(DESTDIR)$(psdir)

uninstall-ps:
	$(RM) $(DESTDIR)$(psdir)/autosprintf.ps


# Documentation in Portable Document Format.

SUFFIXES = .pdf

install-pdf: autosprintf.pdf
	$(mkinstalldirs) $(DESTDIR)$(pdfdir)
	$(INSTALL_DATA) `if test -f autosprintf.pdf; then echo .; else echo $(srcdir); fi`/autosprintf.pdf $(DESTDIR)$(pdfdir)/autosprintf.pdf

installdirs-pdf:
	$(mkinstalldirs) $(DESTDIR)$(pdfdir)

uninstall-pdf:
	$(RM) $(DESTDIR)$(pdfdir)/autosprintf.pdf


# Documentation in HTML format.

TEXI2HTML = @PERL@ $(srcdir)/texi2html

html-local: autosprintf_all.html

# Override of automake's definition.
# We want to use texi2html, not makeinfo --html.
autosprintf_all.html: autosprintf.texi
	$(TEXI2HTML) -expandinfo -number -monolithic `if test -f autosprintf.texi; then echo autosprintf.texi; else echo $(srcdir)/autosprintf.texi; fi`
	mv autosprintf.html autosprintf_all.html

install-html: autosprintf_all.html
	$(mkinstalldirs) $(DESTDIR)$(htmldir)
	$(INSTALL_DATA) `if test -f autosprintf_all.html; then echo .; else echo $(srcdir); fi`/autosprintf_all.html $(DESTDIR)$(htmldir)/autosprintf.html

installdirs-html:
	$(mkinstalldirs) $(DESTDIR)$(htmldir)

uninstall-html:
	$(RM) $(DESTDIR)$(htmldir)/autosprintf.html

EXTRA_DIST += texi2html autosprintf_all.html


# VMS support.

config.h_vms: config.h.in
	(echo '#include <vms_jackets.h>'; \
	 sed -e 's/#undef HAVE_ALLOCA$$/#define HAVE_ALLOCA 1/' \
	     -e 's/#undef HAVE_DLFCN_H$$/#define HAVE_DLFCN_H 1/' \
	     -e 's/#undef HAVE_INTTYPES_H$$/#define HAVE_INTTYPES_H 1/' \
	     -e 's/#undef HAVE_LONG_DOUBLE$$/#define HAVE_LONG_DOUBLE 1/' \
	     -e 's/#undef HAVE_LONG_LONG$$/#define HAVE_LONG_LONG 1/' \
	     -e 's/#undef HAVE_MEMORY_H$$/#define HAVE_MEMORY_H 1/' \
	     -e 's/#undef HAVE_POSIX_PRINTF$$/#define HAVE_POSIX_PRINTF 1/' \
	     -e 's/#undef HAVE_PTRDIFF_T$$/#define HAVE_PTRDIFF_T 1/' \
	     -e 's/#undef HAVE_STDLIB_H$$/#define HAVE_STDLIB_H 1/' \
	     -e 's/#undef HAVE_STRINGS_H$$/#define HAVE_STRINGS_H 1/' \
	     -e 's/#undef HAVE_STRING_H$$/#define HAVE_STRING_H 1/' \
	     -e 's/#undef HAVE_SYS_STAT_H$$/#define HAVE_SYS_STAT_H 1/' \
	     -e 's/#undef HAVE_SYS_TYPES_H$$/#define HAVE_SYS_TYPES_H 1/' \
	     -e 's/#undef HAVE_UNISTD_H$$/#define HAVE_UNISTD_H 1/' \
	     -e 's/#undef HAVE_WCHAR_T$$/#define HAVE_WCHAR_T 1/' \
	     -e 's/#undef HAVE_WCSLEN$$/#define HAVE_WCSLEN 1/' \
	     -e 's/#undef HAVE_WINT_T$$/#define HAVE_WINT_T 1/' \
	     -e 's/#undef STDC_HEADERS$$/#define STDC_HEADERS 1/' \
	   < $(srcdir)/config.h.in; echo; echo '#define alloca __ALLOCA') > $@

EXTRA_DIST += Makefile.vms config.h_vms


# Woe32 support.

config.h.msvc: config.h.in
	sed -e 's/#undef HAVE_ALLOCA$$/#define HAVE_ALLOCA 1/' \
	    -e 's/#undef HAVE_DECL__SNPRINTF$$/#define HAVE_DECL__SNPRINTF 1/' \
	    -e 's/#undef HAVE_LONG_DOUBLE$$/#define HAVE_LONG_DOUBLE 1/' \
	    -e 's/#undef HAVE_PTRDIFF_T$$/#define HAVE_PTRDIFF_T 1/' \
	    -e 's/#undef HAVE_SNPRINTF$$/#define HAVE_SNPRINTF 1/' \
	    -e 's/#undef HAVE_STDLIB_H$$/#define HAVE_STDLIB_H 1/' \
	    -e 's/#undef HAVE_STRING_H$$/#define HAVE_STRING_H 1/' \
	    -e 's/#undef HAVE_WCHAR_T$$/#define HAVE_WCHAR_T 1/' \
	    -e 's/#undef HAVE_WCSLEN$$/#define HAVE_WCSLEN 1/' \
	    -e 's/#undef HAVE_WINT_T$$/#define HAVE_WINT_T 1/' \
	    -e 's/#undef SIZE_MAX$$/#define SIZE_MAX 4294967295U/' \
	    -e 's/#undef inline$$/#define inline __inline/' \
	  < $(srcdir)/config.h.in > $@

autosprintf.h.msvc-shared: autosprintf.h.in windows/dllexport.h
	sed -e 's/extern \([^"]\)/extern LIBASPRINTF_DLL_EXPORTED \1/' \
	    -e 's/class /class LIBASPRINTF_DLL_EXPORTED /' \
	    -e 's/struct /struct LIBASPRINTF_DLL_EXPORTED /' \
	    -e '/#define _AUTOSPRINTF_H/r windows/dllexport.h' < $(srcdir)/autosprintf.h.in > $@

EXTRA_DIST += README.woe32 Makefile.msvc config.h.msvc autosprintf.h.msvc-shared windows/dllexport.h windows/asprintf.rc
