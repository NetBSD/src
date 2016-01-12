## Makefile for the gettext-runtime/libasprintf subdirectory of GNU gettext
## Copyright (C) 2002-2006 Free Software Foundation, Inc.
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
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
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
libasprintf_la_LDFLAGS = @LTNOUNDEF@
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

# The install-dvi target is already defined by automake.

installdirs-dvi:
	$(mkdir_p) $(DESTDIR)$(dvidir)

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

# The install-ps target is already defined by automake.

installdirs-ps:
	$(mkdir_p) $(DESTDIR)$(psdir)

uninstall-ps:
	$(RM) $(DESTDIR)$(psdir)/autosprintf.ps


# Documentation in Portable Document Format.

SUFFIXES = .pdf

# The install-pdf target is already defined by automake.

installdirs-pdf:
	$(mkdir_p) $(DESTDIR)$(pdfdir)

uninstall-pdf:
	$(RM) $(DESTDIR)$(pdfdir)/autosprintf.pdf


# Documentation in HTML format.

TEXI2HTML = @PERL@ $(srcdir)/texi2html

HTMLS = autosprintf_all.html

html-local: $(HTMLS)

# Override of automake's definition.
# We want to use texi2html, not makeinfo --html.
autosprintf_all.html: autosprintf.texi
	$(TEXI2HTML) -expandinfo -number -monolithic `if test -f autosprintf.texi; then echo autosprintf.texi; else echo $(srcdir)/autosprintf.texi; fi`
	mv autosprintf.html autosprintf_all.html

# The install-html target is already defined by automake.

installdirs-html:
	$(mkdir_p) $(DESTDIR)$(htmldir)

uninstall-html:
	$(RM) $(DESTDIR)$(htmldir)/autosprintf.html

EXTRA_DIST += texi2html autosprintf_all.html


# Woe32 support.

EXTRA_DIST += README.woe32
