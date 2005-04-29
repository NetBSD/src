## Makefile for the gettext-tools/tests subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2001-2005 Free Software Foundation, Inc.
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

TESTS = gettext-1 gettext-2 \
	msgattrib-1 msgattrib-2 msgattrib-3 msgattrib-4 msgattrib-5 \
	msgattrib-6 msgattrib-7 msgattrib-8 msgattrib-9 msgattrib-10 \
	msgattrib-11 msgattrib-12 msgattrib-13 msgattrib-14 \
	msgattrib-properties-1 \
	msgcat-1 msgcat-2 msgcat-3 msgcat-4 msgcat-5 msgcat-6 msgcat-7 \
	msgcat-8 msgcat-9 msgcat-10 msgcat-11 \
	msgcat-properties-1 msgcat-properties-2 \
	msgcat-stringtable-1 \
	msgcmp-1 msgcmp-2 msgcmp-3 \
	msgcomm-1 msgcomm-2 msgcomm-3 msgcomm-4 msgcomm-5 msgcomm-6 msgcomm-7 \
	msgcomm-8 msgcomm-9 msgcomm-10 msgcomm-11 msgcomm-12 msgcomm-13 \
	msgcomm-14 msgcomm-15 msgcomm-16 msgcomm-17 msgcomm-18 msgcomm-19 \
	msgcomm-20 msgcomm-21 msgcomm-22 msgcomm-23 msgcomm-24 \
	msgconv-1 msgconv-2 msgconv-3 msgconv-4 \
	msgen-1 msgen-2 \
	msgexec-1 msgexec-2 msgexec-3 \
	msgfilter-1 msgfilter-2 msgfilter-3 \
	msgfmt-1 msgfmt-2 msgfmt-3 msgfmt-4 msgfmt-5 msgfmt-6 msgfmt-7 \
	msgfmt-8 msgfmt-9 msgfmt-10 msgfmt-11 msgfmt-12 msgfmt-13 \
	msgfmt-properties-1 \
	msgfmt-qt-1 \
	msggrep-1 msggrep-2 msggrep-3 msggrep-4 msggrep-5 msggrep-6 \
	msginit-1 \
	msgmerge-1 msgmerge-2 msgmerge-3 msgmerge-4 msgmerge-5 msgmerge-6 \
	msgmerge-7 msgmerge-8 msgmerge-9 msgmerge-10 msgmerge-11 msgmerge-12 \
	msgmerge-13 msgmerge-14 msgmerge-15 msgmerge-16 msgmerge-17 \
	msgmerge-compendium-1 msgmerge-compendium-2 msgmerge-compendium-3 \
	msgmerge-compendium-4 \
	msgmerge-properties-1 msgmerge-properties-2 \
	msgmerge-update-1 msgmerge-update-2 msgmerge-update-3 \
	msgunfmt-1 \
	msgunfmt-csharp-1 \
	msgunfmt-java-1 \
	msgunfmt-properties-1 \
	msgunfmt-tcl-1 \
	msguniq-1 msguniq-2 msguniq-3 msguniq-4 \
	xgettext-1 xgettext-2 xgettext-3 xgettext-4 xgettext-5 xgettext-6 \
	xgettext-7 xgettext-8 \
	xgettext-awk-1 \
	xgettext-c-1 xgettext-c-2 xgettext-c-3 xgettext-c-4 xgettext-c-5 \
	xgettext-c-6 xgettext-c-7 xgettext-c-8 xgettext-c-9 \
	xgettext-csharp-1 xgettext-csharp-2 xgettext-csharp-3 \
	xgettext-csharp-4 xgettext-csharp-5 \
	xgettext-elisp-1 \
	xgettext-glade-1 xgettext-glade-2 xgettext-glade-3 \
	xgettext-java-1 xgettext-java-2 xgettext-java-3 xgettext-java-4 \
	xgettext-java-5 \
	xgettext-librep-1 \
	xgettext-lisp-1 \
	xgettext-objc-1 xgettext-objc-2 \
	xgettext-perl-1 xgettext-perl-2 xgettext-perl-3 xgettext-perl-4 \
	xgettext-perl-5 \
	xgettext-php-1 \
	xgettext-po-1 \
	xgettext-properties-1 \
	xgettext-python-1 xgettext-python-2 \
	xgettext-scheme-1 \
	xgettext-sh-1 xgettext-sh-2 xgettext-sh-3 \
	xgettext-smalltalk-1 \
	xgettext-stringtable-1 \
	xgettext-tcl-1 xgettext-tcl-2 \
	xgettext-ycp-1 xgettext-ycp-2 \
	format-awk-1 format-awk-2 \
	format-c-1 format-c-2 format-c-3 format-c-4 format-c-5 \
	format-csharp-1 format-csharp-2 \
	format-elisp-1 format-elisp-2 \
	format-gcc-internal-1 format-gcc-internal-2 \
	format-java-1 format-java-2 \
	format-librep-1 format-librep-2 \
	format-lisp-1 format-lisp-2 \
	format-php-1 format-php-2 \
	format-python-1 format-python-2 \
	format-pascal-1 format-pascal-2 \
	format-perl-1 format-perl-2 \
	format-perl-brace-1 format-perl-brace-2 \
	format-perl-mixed-1 format-perl-mixed-2 \
	format-qt-1 format-qt-2 \
	format-scheme-1 format-scheme-2 \
	format-sh-1 format-sh-2 \
	format-tcl-1 format-tcl-2 \
	format-ycp-1 format-ycp-2 \
	plural-1 plural-2 \
	lang-c lang-c++ lang-objc lang-sh lang-bash lang-python-1 \
	lang-python-2 lang-clisp lang-elisp lang-librep lang-guile \
	lang-smalltalk lang-java lang-csharp lang-gawk lang-pascal \
	lang-ycp lang-tcl lang-perl-1 lang-perl-2 lang-php lang-po lang-rst

EXTRA_DIST += $(TESTS) \
	test.mo xg-c-1.ok.po mex-test2.ok msguniq-a.in msguniq-a.inp \
	msguniq-a.out qttest_pl.po qttest_pl.qm ChangeLog.0

XGETTEXT = ../src/xgettext

# For debugging memory leaks and memory allocation bugs.
# You should build with --disable-shared when using valgrind.
CHECKER =
#CHECKER = valgrind --tool=memcheck --suppressions=$(srcdir)/../lib/allocsa.valgrind --num-callers=20 --leak-check=yes --leak-resolution=high --show-reachable=yes
#CHECKER = valgrind --tool=massif --format=html --depth=10 --alloc-fn=xmalloc --alloc-fn=xrealloc --stacks=no

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) \
		    PATH=.:../src:../../gettext-runtime/src:$(top_srcdir)/../gettext-runtime/src:$$PATH \
		    GETTEXT="$(CHECKER) tstgettext" \
		    NGETTEXT="$(CHECKER) tstngettext" \
		    XGETTEXT="$(CHECKER) xgettext" \
		    MSGATTRIB="$(CHECKER) msgattrib" \
		    MSGCAT="$(CHECKER) msgcat" \
		    MSGCMP="$(CHECKER) msgcmp" \
		    MSGCOMM="$(CHECKER) msgcomm" \
		    MSGCONV="$(CHECKER) msgconv" \
		    MSGEN="$(CHECKER) msgen" \
		    MSGEXEC="$(CHECKER) msgexec" \
		    MSGFILTER="$(CHECKER) msgfilter" \
		    MSGFMT="$(CHECKER) msgfmt" \
		    MSGGREP="$(CHECKER) msggrep" \
		    MSGINIT="$(CHECKER) msginit" \
		    MSGMERGE="$(CHECKER) msgmerge" \
		    MSGUNFMT="$(CHECKER) msgunfmt" \
		    MSGUNIQ="$(CHECKER) msguniq" \
		    OBJEXT='@OBJEXT@' EXEEXT='@EXEEXT@' \
		    LIBTOOL='@LIBTOOL@' \
		    CC='@CC@' CFLAGS='@CFLAGS@' \
		    CXX='@CXX@' CXXFLAGS='@CXXFLAGS@' \
		    CPPFLAGS='@CPPFLAGS@' LDFLAGS='@LDFLAGS@' \
		    LTLIBINTL='@LTLIBINTL@' \
		    TESTJAVA='@TESTJAVA@' \
		    TESTCSHARP='@TESTCSHARP@' \
		    TESTLIBASPRINTF='@TESTLIBASPRINTF@' \
		    LOCALE_FR='@LOCALE_FR@' \
		    LOCALE_FR_UTF8='@LOCALE_FR_UTF8@' \
		    host_os='@host_os@' \
		    CONFIG_SHELL='$(SHELL)' \
		    $(SHELL)

xg-c-1.ok.po: $(top_srcdir)/src/xgettext.c $(top_srcdir)/src/msgfmt.c
	$(XGETTEXT) -d xg-c-1.ok -p $(srcdir) \
	  -k_ --omit-header --no-location \
	  $(top_srcdir)/src/xgettext.c $(top_srcdir)/src/msgfmt.c

# Three auxiliary programs used by the tests.
localedir = $(datadir)/locale
AM_CPPFLAGS = \
  -I.. \
  -I../lib -I$(top_srcdir)/lib \
  -I../intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
# INTL_MACOSX_LIBS is needed because the programs depend on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
LDADD = $(LDADD_@USE_INCLUDED_LIBINTL@) @INTL_MACOSX_LIBS@
LDADD_yes = ../intl/libintl.la
LDADD_no = ../intl/libgnuintl.la @LTLIBINTL@
noinst_PROGRAMS = tstgettext tstngettext testlocale cake fc3 fc4 fc5
tstgettext_SOURCES = tstgettext.c setlocale.c
tstgettext_CFLAGS = -DINSTALLDIR=\".\"
tstgettext_LDADD = ../lib/libgettextlib.la $(LDADD)
tstngettext_SOURCES = tstngettext.c setlocale.c
tstngettext_CFLAGS = -DINSTALLDIR=\".\"
tstngettext_LDADD = ../lib/libgettextlib.la $(LDADD)
testlocale_SOURCES = testlocale.c
cake_SOURCES = plural-1-prg.c setlocale.c
cake_LDADD = ../lib/libgettextlib.la $(LDADD)
fc3_SOURCES = format-c-3-prg.c setlocale.c
fc3_LDADD = ../lib/libgettextlib.la $(LDADD)
fc4_SOURCES = format-c-4-prg.c setlocale.c
fc4_LDADD = ../lib/libgettextlib.la $(LDADD)
fc5_SOURCES = format-c-5-prg.c
fc5_LDADD = ../lib/libgettextlib.la $(LDADD)

# Clean up after Solaris cc.
clean-local:
	rm -rf SunWS_cache


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
