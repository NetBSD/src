## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2021 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I . -I .. -I ../.. 

CCSOURCES = \
	Application.cc \
	BaseMetric.cc \
	BaseMetricTreeNode.cc \
	CallStack.cc \
	CatchOutOfMemory.cc \
	ClassFile.cc \
	Command.cc \
	CompCom.cc \
	DataObject.cc \
	DataSpace.cc \
	Data_window.cc \
	DataStream.cc \
	DbeApplication.cc \
	DbeFile.cc \
	DbeJarFile.cc \
	DbeLock.cc \
	DbeSession.cc \
	DbeThread.cc \
	DbeView.cc \
	DerivedMetrics.cc \
	Disasm.cc \
	Dwarf.cc \
	DwarfLib.cc \
	Elf.cc \
	Emsg.cc \
	Experiment.cc \
	Exp_Layout.cc \
	ExpGroup.cc \
	Expression.cc \
	FileData.cc \
	Filter.cc \
	FilterSet.cc \
	Function.cc \
	HeapMap.cc \
	HeapData.cc \
	HeapActivity.cc \
	Hist_data.cc \
	IndexObject.cc \
	IOActivity.cc \
	LoadObject.cc \
	MachineModel.cc \
	MemObject.cc \
	MemorySpace.cc \
	Metric.cc \
	MetricList.cc \
	Module.cc \
	Ovw_data.cc \
	PRBTree.cc \
	PathTree.cc \
	PreviewExp.cc \
	Print.cc \
	SAXParserFactory.cc \
	Sample.cc \
	Settings.cc \
	SourceFile.cc \
	Stabs.cc \
	Stats_data.cc \
	StringBuilder.cc \
	Table.cc \
	QLParser.tab.cc \
	dbe_collctrl.cc \
	i18n.cc \
	parse.cc \
	UserLabel.cc \
	util.cc \
	Dbe.cc \
	$(NULL)

CSOURCES = \
	dbe_hwcdrv.c \
	dbe_hwcfuncs.c \
	dbe_hwctable.c \
	dbe_memmgr.c \
	gethrtime.c \
	$(NULL)

LIBGPROFNG = libgprofng.la

# This is where we get zlib from.  zlibdir is -L../zlib and zlibinc is
# -I../zlib, unless we were configured with --with-system-zlib, in which
# case both are empty.
ZLIB = @zlibdir@ -lz
ZLIBINC = @zlibinc@

LOCALEDIR = @localedir@
SYSCONFDIR = @sysconfdir@

AM_CPPFLAGS = $(GPROFNG_CPPFLAGS) -DLOCALEDIR=\"$(LOCALEDIR)\" \
	-DSYSCONFDIR=\"$(SYSCONFDIR)\" \
	-I.. -I$(srcdir) \
	-I$(srcdir)/../common \
	-I$(srcdir)/../../include -I$(srcdir)/../../opcodes \
	-I../../bfd -I$(srcdir)/../../bfd $(ZLIBINC)
AM_CFLAGS = $(GPROFNG_CFLAGS) $(PTHREAD_CFLAGS) \
	$(GPROFNG_NO_SWITCH_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS)

QLParser.tab.cc QLParser.tab.hh: QLParser.yy
	$(BISON) $^

BUILT_SOURCES = QLParser.tab.hh
EXTRA_DIST = QLParser.yy $(man_MANS)


pkglib_LTLIBRARIES = $(LIBGPROFNG)
libgprofng_la_SOURCES = $(CCSOURCES) $(CSOURCES)
libgprofng_la_LDFLAGS = -version-info 0:0:0

# Pass -lpthread instead of $(PTHREAD_LIBS) due to $(PTHREAD_LIBS) being empty
# when -nostdlib is passed to libtool.
# See bug 29364 - libgprofng.so: needs to link against -pthread
libgprofng_la_LIBADD = $(top_builddir)/../opcodes/libopcodes.la \
	$(top_builddir)/../bfd/libbfd.la \
	$(GPROFNG_LIBADD) \
	-lpthread -ldl

dbedir = $(SYSCONFDIR)
dbe_DATA = $(srcdir)/gprofng.rc


bin_PROGRAMS = gp-archive gp-collect-app gprofng gp-display-text gp-display-src

gp_archive_SOURCES = gp-archive.cc ArchiveExp.cc
gp_archive_LDADD = $(LIBGPROFNG) $(CLOCK_GETTIME_LINK) $(ZLIB)

gp_collect_app_SOURCES = gp-collect-app.cc checks.cc envsets.cc count.cc
gp_collect_app_LDADD = $(LIBGPROFNG) $(CLOCK_GETTIME_LINK) $(ZLIB)

gprofng_SOURCES = gprofng.cc
gprofng_LDADD = $(LIBGPROFNG) $(CLOCK_GETTIME_LINK) $(ZLIB)

gp_display_src_SOURCES = gp-display-src.cc
gp_display_src_LDADD = $(LIBGPROFNG) $(CLOCK_GETTIME_LINK) $(ZLIB)

gp_display_text_SOURCES = gp-display-text.cc ipc.cc ipcio.cc
gp_display_text_LDADD = $(LIBGPROFNG) $(CLOCK_GETTIME_LINK) $(ZLIB)


if BUILD_MAN

man_MANS = gprofng.1 \
	gp-archive.1 \
	gp-collect-app.1 \
	gp-display-src.1 \
	gp-display-text.1

MAINTAINERCLEANFILES = $(man_MANS)

# The man pages depend on the version number and on a help2man include file.
common_mandeps = $(top_srcdir)/../bfd/version.m4

# Use -o so that the `missing' program can infer the output file.
# Embolden subcommand names in the output, and include a SEE ALSO.
# Arrange to regenerate the output if we have help2man, but leave the
# disted output there otherwise.
# Some extra annoying complexity is in place so that people without
# help2man dno't accidentally overwrite the manpage.

INFO_PAGE            = "gprofng"
MANUAL               = "User Commands"
TEXT_GPROFNG         = "the driver for the gprofng tool suite"
TEXT_GP_ARCHIVE      = "archive gprofng experiment data"
TEXT_GP_COLLECT_APP  = "collect performance data for the target application"
TEXT_GP_DISPLAY_SRC  = "display the source code, optionally interleaved with the disassembly of the target object"
TEXT_GP_DISPLAY_TEXT = "display the performance data in plain text format"

HELP2MAN_OPT = --libtool --no-info --info-page=$(INFO_PAGE) --manual=$(MANUAL)
H2M_FILTER = | sed 's/\.TP/\.TP\n.B/' | sed 's/Commands:/\.SH COMMANDS/' \
  | sed 's/See also:/\.SH SEE ALSO/' | sed 's/Documentation:/.SH DOCUMENTATION/' \
  | sed 's/Limitations:/.SH LIMITATIONS/'

gprofng.1: $(srcdir)/gprofng.cc $(common_mandeps) | ./gprofng$(EXEEXT)
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GPROFNG) ./gprofng$(EXEEXT) $(H2M_FILTER) > $@

gp-archive.1: $(srcdir)/gp-archive.cc $(common_mandeps) | ./gp-archive$(EXEEXT)
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GP_ARCHIVE) ./gp-archive$(EXEEXT) $(H2M_FILTER) > $@

gp-collect-app.1: $(srcdir)/gp-collect-app.cc $(common_mandeps) | ./gp-collect-app$(EXEEXT)
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GP_COLLECT_APP) ./gp-collect-app$(EXEEXT) $(H2M_FILTER) > $@

gp-display-src.1: $(srcdir)/gp-display-src.cc $(srcdir)/Command.cc \
		$(common_mandeps) | ./gp-display-src$(EXEEXT)
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GP_DISPLAY_SRC) ./gp-display-src$(EXEEXT) $(H2M_FILTER) > $@

gp-display-text.1: $(srcdir)/gp-display-text.cc $(srcdir)/Command.cc \
		$(common_mandeps) | ./gp-display-text$(EXEEXT)
	$(AM_V_GEN)_BUILDING_MANPAGE=1 $(HELP2MAN) $(HELP2MAN_OPT) \
	  --name=$(TEXT_GP_DISPLAY_TEXT) ./gp-display-text$(EXEEXT) $(H2M_FILTER) > $@

endif

# Distribution involves building the binaries to generate the manpage,
# so ensure that the necessary libraries are built at dist time.
dist-hook: $(LIBGPROFNG)

.PHONY: install-data-local

install-data-local: install-pkglibLTLIBRARIES
	rm -f $(DESTDIR)/$(pkglibdir)/*.la $(DESTDIR)/$(pkglibdir)/*.a
