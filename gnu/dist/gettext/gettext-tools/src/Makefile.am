## Makefile for the gettext-tools/src subdirectory of GNU gettext
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
CLEANFILES =
DISTCLEANFILES =

RM = rm -f

bin_PROGRAMS = \
msgcmp msgfmt msgmerge msgunfmt xgettext \
msgattrib msgcat msgcomm msgconv msgen msgexec msgfilter msggrep msginit msguniq

noinst_PROGRAMS = hostname urlget

lib_LTLIBRARIES = libgettextsrc.la libgettextpo.la

include_HEADERS = gettext-po.h

noinst_HEADERS = pos.h message.h po-error.h po-gram.h po-charset.h \
po-lex.h open-po.h \
read-po-abstract.h read-po.h read-properties.h read-stringtable.h \
str-list.h \
write-po.h write-properties.h write-stringtable.h \
dir-list.h file-list.h po-gram-gen.h po-gram-gen2.h \
msgl-charset.h msgl-equal.h msgl-iconv.h msgl-ascii.h msgl-cat.h \
msgl-english.h msgfmt.h msgunfmt.h plural-count.h \
read-mo.h write-mo.h \
read-java.h write-java.h \
read-csharp.h write-csharp.h \
read-resources.h write-resources.h \
read-tcl.h write-tcl.h \
write-qt.h \
po-time.h plural-table.h format.h \
xgettext.h x-c.h x-po.h x-sh.h x-python.h x-lisp.h x-elisp.h x-librep.h \
x-scheme.h x-smalltalk.h x-java.h x-properties.h x-csharp.h x-awk.h x-ycp.h \
x-tcl.h x-perl.h x-php.h x-stringtable.h x-rst.h x-glade.h

EXTRA_DIST += FILES project-id ChangeLog.0

localedir = $(datadir)/locale
aliaspath = $(localedir)
jardir = $(datadir)/gettext
pkgdatadir = $(datadir)/gettext
projectsdir = $(pkgdatadir)/projects
pkglibdir = $(libdir)/gettext

AM_CPPFLAGS = \
  -I. -I$(srcdir) \
  -I.. \
  -I$(top_srcdir)/libgrep \
  -I$(top_srcdir)/libuniname \
  -I../lib -I$(top_srcdir)/lib \
  -I../intl -I$(top_srcdir)/../gettext-runtime/intl
DEFS = \
  -DLOCALEDIR=\"$(localedir)\" \
  -DLOCALE_ALIAS_PATH=\"$(aliaspath)\" \
  -DUSEJEXE=$(USEJEXE) \
  -DGETTEXTJEXEDIR=\"$(pkglibdir)\" \
  -DGETTEXTJAR=\"$(jardir)/gettext.jar\" \
  -DLIBDIR=\"$(libdir)\" \
  -DGETTEXTDATADIR=\"$(pkgdatadir)\" \
  -DPROJECTSDIR=\"$(projectsdir)\" @DEFS@
LDADD = ../lib/libgettextlib.la @LTLIBINTL@

SED = sed
YACC = @YACC@ -d
GCJ = @GCJ@
GCJFLAGS = @GCJFLAGS@
JAR = @JAR@
JAVACOMP = $(SHELL) ../lib/javacomp.sh
CSHARPCOMP = $(SHELL) ../lib/csharpcomp.sh
CSHARPCOMPFLAGS = -O -g


# All programs deal with message lists.
# All programs must read PO files. (msgunfmt also, for read-java.c,
# read-csharp.c and read-resources.c.)
# message.c -> str-list.c.
# (read-po-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> str-list.c.
# (read-po-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> open-po.c -> dir-list.c -> str-list.c.
# (read-po-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> po-charset.c.
# (read-po-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> message.c -> str-list.c.
COMMON_SOURCE = message.c po-error.c \
read-po-abstract.c po-lex.c po-gram-gen.y po-charset.c \
read-properties.c read-stringtable.c open-po.c dir-list.c str-list.c

# xgettext and msgfmt deal with format strings.
FORMAT_SOURCE = format.c format-invalid.h \
format-c.c format-sh.c format-python.c format-lisp.c format-elisp.c \
format-librep.c format-scheme.c format-java.c format-csharp.c format-awk.c \
format-pascal.c format-ycp.c format-tcl.c format-perl.c format-perl-brace.c \
format-php.c format-gcc-internal.c format-qt.c

# libgettextsrc contains all code that is needed by at least two programs.
libgettextsrc_la_SOURCES = \
$(COMMON_SOURCE) read-po.c write-properties.c write-stringtable.c write-po.c \
msgl-ascii.c msgl-iconv.c msgl-equal.c msgl-cat.c msgl-english.c file-list.c \
msgl-charset.c po-time.c plural.c plural-table.c $(FORMAT_SOURCE)

# libgettextpo contains the public API for PO files.
libgettextpo_la_SOURCES = gettext-po.c
# Libtool's library version information for libgettextpo.
# See the libtool documentation, section "Library interface versions".
LTV_CURRENT=1
LTV_REVISION=0
LTV_AGE=1

# msggrep needs pattern matching.
LIBGREP = ../libgrep/libgrep.a

# x-python needs table of Unicode character names.
LIBUNINAME = ../libuniname/libuniname.a

# Source dependencies.
msgcmp_SOURCES = msgcmp.c
msgfmt_SOURCES = msgfmt.c \
  write-mo.c write-java.c write-csharp.c write-resources.c write-tcl.c \
  write-qt.c plural-eval.c
msgmerge_SOURCES = msgmerge.c plural-count.c
msgunfmt_SOURCES = msgunfmt.c \
  read-mo.c read-java.c read-csharp.c read-resources.c read-tcl.c
xgettext_SOURCES = xgettext.c \
  x-c.c x-po.c x-sh.c x-python.c x-lisp.c x-elisp.c x-librep.c x-scheme.c \
  x-smalltalk.c x-java.c x-csharp.c x-awk.c x-ycp.c x-tcl.c x-perl.c x-php.c \
  x-rst.c x-glade.c
msgattrib_SOURCES = msgattrib.c
msgcat_SOURCES = msgcat.c
msgcomm_SOURCES = msgcomm.c
msgconv_SOURCES = msgconv.c
msgen_SOURCES = msgen.c
msgexec_SOURCES = msgexec.c
msgfilter_SOURCES = msgfilter.c
msggrep_SOURCES = msggrep.c
msginit_SOURCES = msginit.c plural-count.c ../../gettext-runtime/intl/localealias.c
msguniq_SOURCES = msguniq.c
hostname_SOURCES = hostname.c
urlget_SOURCES = urlget.c

# How to build libgettextsrc.la.
# Need ../lib/libgettextlib.la.
# Need @LTLIBINTL@ because many source files use gettext().
# Need @LTLIBICONV@ because po-charset.c, po-lex.c, msgl-iconv.c, write-po.c
# use iconv().
libgettextsrc_la_LDFLAGS = \
  -release @VERSION@ \
  ../lib/libgettextlib.la @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined

# No need to install libgettextsrc.a, except on AIX.
install-exec-local: install-libLTLIBRARIES install-exec-clean
install-exec-clean:
	case "@host_os@" in \
	  aix*) ;; \
	  *) $(RM) $(DESTDIR)$(libdir)/libgettextsrc.a ;; \
	esac

# How to build libgettextpo.la.
# Need libgettextsrc.la.
# Need ../lib/libgettextlib.la because of xmalloc.
libgettextpo_la_LDFLAGS = \
  -version-info $(LTV_CURRENT):$(LTV_REVISION):$(LTV_AGE) \
  -rpath $(libdir) \
  libgettextsrc.la ../lib/libgettextlib.la @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined
# Build order. Only needed for "make -j[N]".
libgettextpo_la_DEPENDENCIES = libgettextsrc.la

# Link dependencies.
# INTL_MACOSX_LIBS is needed because the programs depend on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
# For msginit, it is also needed because of localename.c.
msgcmp_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgmerge_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgunfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
xgettext_LDADD = $(LIBUNINAME) libgettextsrc.la @INTL_MACOSX_LIBS@ @LTLIBEXPAT@
msgattrib_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgcat_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgcomm_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgconv_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgen_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgexec_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msgfilter_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@
msggrep_LDADD = $(LIBGREP) libgettextsrc.la @INTL_MACOSX_LIBS@
msginit_LDADD = ../intl/localename.@INTL_LIBTOOL_SUFFIX_PREFIX@o \
                libgettextsrc.la @INTL_MACOSX_LIBS@
msguniq_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@

# Specify installation directory, for --enable-relocatable.
msgcmp_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgfmt_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgmerge_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgunfmt_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
xgettext_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgattrib_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgcat_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgcomm_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgconv_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgen_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgexec_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msgfilter_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msggrep_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msginit_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
msguniq_CFLAGS = -DINSTALLDIR=\"$(bindir)\"
hostname_CFLAGS = -DINSTALLDIR=\"$(pkglibdir)\"
urlget_CFLAGS = -DINSTALLDIR=\"$(pkglibdir)\"
if RELOCATABLE_VIA_LD
msgcmp_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgfmt_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgmerge_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgunfmt_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
xgettext_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgattrib_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgcat_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgcomm_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgconv_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgen_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgexec_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgfilter_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msggrep_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msginit_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msguniq_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
hostname_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibdir)`
urlget_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibdir)`
endif

# Special rules for bison and flex generated files.

BUILT_SOURCES = \
  po-gram-gen.c po-gram-gen.h po-gram-gen2.h

po-lex.o po-lex.lo: po-gram-gen2.h
po-gram-gen2.h: po-gram-gen.h
	$(SED) -e 's/yy/po_gram_/g' -e 's/extern /extern DLL_VARIABLE /' $(srcdir)/po-gram-gen.h > $@-tmp
	mv $@-tmp $@


# Special rules for installation of auxiliary programs.

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) hostname$(EXEEXT) $(DESTDIR)$(pkglibdir)/hostname$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) urlget$(EXEEXT) $(DESTDIR)$(pkglibdir)/urlget$(EXEEXT)
	$(INSTALL_SCRIPT) user-email $(DESTDIR)$(pkglibdir)/user-email
	$(INSTALL_SCRIPT) $(srcdir)/project-id $(DESTDIR)$(pkglibdir)/project-id

installdirs-local:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)

uninstall-local:
	$(RM) $(DESTDIR)$(pkglibdir)/hostname$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/urlget$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/user-email
	$(RM) $(DESTDIR)$(pkglibdir)/project-id

DISTCLEANFILES += user-email


# Special rules for Java compilation.

USEJEXE = $(USEJEXE_@BUILDJAVAEXE@)
USEJEXE_yes = 1
USEJEXE_no = 0

all-local: all-java-@BUILDJAVAEXE@
all-java-yes: gnu.gettext.DumpResource$(EXEEXT) gnu.gettext.GetURL$(EXEEXT)
all-java-no: all-java-no-@BUILDJAVA@
all-java-no-yes: gettext.jar
all-java-no-no:

gnu.gettext.DumpResource$(EXEEXT): $(srcdir)/gnu/gettext/DumpResource.java
	$(GCJ) $(GCJFLAGS) $(srcdir)/gnu/gettext/DumpResource.java --main=gnu.gettext.DumpResource -o $@

gnu.gettext.GetURL$(EXEEXT): $(srcdir)/gnu/gettext/GetURL.java
	$(GCJ) $(GCJFLAGS) $(srcdir)/gnu/gettext/GetURL.java --main=gnu.gettext.GetURL -o $@

gnu/gettext/DumpResource.class: $(srcdir)/gnu/gettext/DumpResource.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/DumpResource.java

gnu/gettext/GetURL.class: $(srcdir)/gnu/gettext/GetURL.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/GetURL.java

gettext.jar: gnu/gettext/DumpResource.class gnu/gettext/GetURL.class
	$(JAR) cf $@ gnu/gettext/DumpResource*.class gnu/gettext/GetURL*.class

EXTRA_DIST += gnu/gettext/DumpResource.java gnu/gettext/GetURL.java

CLEANFILES += gnu.gettext.DumpResource$(EXEEXT) gnu.gettext.GetURL$(EXEEXT) \
              gettext.jar gnu/gettext/*.class

install-exec-local: install-exec-java-@BUILDJAVAEXE@
install-exec-java-yes: all-java-yes
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) gnu.gettext.DumpResource$(EXEEXT) $(DESTDIR)$(pkglibdir)/gnu.gettext.DumpResource$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) gnu.gettext.GetURL$(EXEEXT) $(DESTDIR)$(pkglibdir)/gnu.gettext.GetURL$(EXEEXT)
install-exec-java-no:

install-data-local: install-data-java-@BUILDJAVAEXE@
install-data-java-yes:
install-data-java-no: install-data-java-no-@BUILDJAVA@
install-data-java-no-yes: all-java-no-yes
	$(INSTALL_DATA) gettext.jar $(DESTDIR)$(jardir)/gettext.jar
install-data-java-no-no:

installdirs-local: installdirs-java-@BUILDJAVAEXE@
installdirs-java-yes:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
installdirs-java-no: installdirs-java-no-@BUILDJAVA@
installdirs-java-no-yes:
	$(mkinstalldirs) $(DESTDIR)$(jardir)
installdirs-java-no-no:

uninstall-local: uninstall-java-@BUILDJAVAEXE@
uninstall-java-yes:
	$(RM) $(DESTDIR)$(pkglibdir)/gnu.gettext.DumpResource$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/gnu.gettext.GetURL$(EXEEXT)
uninstall-java-no: uninstall-java-no-@BUILDJAVA@
uninstall-java-no-yes:
	$(RM) $(DESTDIR)$(jardir)/gettext.jar
uninstall-java-no-no:


# Special rules for C# auxiliary programs.

EXTRA_DIST += msgfmt.cs msgunfmt.cs

CLEANFILES += msgfmt.net.exe msgunfmt.net.exe

all-local: all-csharp-@BUILDCSHARP@
all-csharp-yes: msgfmt.net.exe msgunfmt.net.exe
all-csharp-no:

msgfmt.net.exe: msgfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ $(srcdir)/msgfmt.cs

msgunfmt.net.exe: msgunfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ -L ../../gettext-runtime/intl-csharp -l GNU.Gettext $(srcdir)/msgunfmt.cs

install-exec-local: install-exec-csharp-@BUILDCSHARP@
install-exec-csharp-yes: all-csharp-yes
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	$(INSTALL_DATA) msgfmt.net.exe $(DESTDIR)$(pkglibdir)/msgfmt.net.exe
	$(INSTALL_DATA) msgunfmt.net.exe $(DESTDIR)$(pkglibdir)/msgunfmt.net.exe
install-exec-csharp-no:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)

installdirs-local: install-csharp
installdirs-csharp:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)

uninstall-local: uninstall-csharp-@BUILDCSHARP@
uninstall-csharp-yes: all-csharp-yes
	$(RM) $(DESTDIR)$(pkglibdir)/msgfmt.net.exe
	$(RM) $(DESTDIR)$(pkglibdir)/msgunfmt.net.exe
uninstall-csharp-no:


# Special rules for Tcl auxiliary program.

EXTRA_DIST += msgunfmt.tcl

install-data-local: install-tcl
install-tcl:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(srcdir)/msgunfmt.tcl $(DESTDIR)$(pkgdatadir)/msgunfmt.tcl

installdirs-local: installdirs-tcl
installdirs-tcl:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)

uninstall-local: uninstall-tcl
uninstall-tcl:
	$(RM) $(DESTDIR)$(pkgdatadir)/msgunfmt.tcl


# Support for relocatability.
RELOCATABLE_LIBRARY_PATH = $(libdir)
RELOCATABLE_SRC_DIR = $(top_srcdir)/lib
RELOCATABLE_BUILD_DIR = ../lib
RELOCATABLE_CONFIG_H_DIR = ..
@SET_RELOCATABLE@


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
