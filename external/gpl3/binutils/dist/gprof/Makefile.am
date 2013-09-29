## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = 1.11 foreign no-dist no-texinfo.tex
ACLOCAL_AMFLAGS = -I .. -I ../config -I ../bfd
TEXINFO_TEX = ../texinfo/texinfo.tex

SUFFIXES = .m

SUBDIRS = po

BASEDIR = $(srcdir)/..
BFDDIR = $(BASEDIR)/bfd
INCDIR	= $(BASEDIR)/include

WARN_CFLAGS = @WARN_CFLAGS@
NO_WERROR = @NO_WERROR@
AM_CFLAGS = $(WARN_CFLAGS)

AM_CPPFLAGS = -DDEBUG -I../bfd -I$(srcdir)/../include \
	-I$(srcdir)/../bfd @INCINTL@ -I. \
	-DLOCALEDIR="\"$(datadir)/locale\""

bin_PROGRAMS = gprof

## Convenience var listing pure sources.
sources = basic_blocks.c call_graph.c cg_arcs.c cg_dfn.c \
	cg_print.c corefile.c gmon_io.c gprof.c hertz.c hist.c source.c \
	search_list.c symtab.c sym_ids.c utils.c \
	i386.c alpha.c vax.c tahoe.c sparc.c mips.c
gprof_SOURCES = $(sources) flat_bl.c bsd_callg_bl.c fsf_callg_bl.c
gprof_DEPENDENCIES = ../bfd/libbfd.la ../libiberty/libiberty.a $(LIBINTL_DEP)
gprof_LDADD = ../bfd/libbfd.la ../libiberty/libiberty.a $(LIBINTL)

noinst_HEADERS = \
	basic_blocks.h call_graph.h cg_arcs.h cg_dfn.h cg_print.h \
	corefile.h gmon.h gmon_io.h gmon_out.h gprof.h hertz.h hist.h \
	search_list.h source.h sym_ids.h symtab.h utils.h

BUILT_SOURCES = flat_bl.c bsd_callg_bl.c fsf_callg_bl.c
EXTRA_DIST    = $(BUILT_SOURCES) bbconv.pl $(man_MANS)

diststuff: $(BUILT_SOURCES) info $(man_MANS)

# We extract version from bfd/configure.in, make sure to rerun configure
# when BFD's version changes.
CONFIG_STATUS_DEPENDENCIES = $(BFDDIR)/configure.in

# This empty rule is a hack against gmake patched by Apple.
%.o:%.m

.m.c:
	awk -f $(srcdir)/gen-c-prog.awk > ./$*.c \
	    FUNCTION=`(echo $*|sed -e 's,.*/,,g' -e 's/_bl//')`_blurb \
	    FILE=$*.m $(srcdir)/$*.m

POTFILES = $(sources) $(noinst_HEADERS)
po/POTFILES.in: @MAINT@ Makefile
	for f in $(POTFILES); do echo $$f; done | LC_ALL=C sort > tmp \
	  && mv tmp $(srcdir)/po/POTFILES.in

MANCONF  = -Dman

TEXI2POD = perl $(srcdir)/../etc/texi2pod.pl $(AM_MAKEINFOFLAGS)

POD2MAN = pod2man --center="GNU" --release="binutils-$(VERSION)" --section=1

info_TEXINFOS = gprof.texi
gprof_TEXINFOS = config.texi
man_MANS = gprof.1

AM_MAKEINFOFLAGS = -I "$(srcdir)" -I "$(top_srcdir)/../bfd/doc" -I ../bfd/doc
TEXI2DVI = texi2dvi -I "$(srcdir)" -I "$(top_srcdir)/../bfd/doc" -I ../bfd/doc

config.texi:
	echo "@set top_srcdir $(top_srcdir)" >> ./config.texi

# Build the man page from the texinfo file
# The sed command removes the no-adjust Nroff command so that
# the man output looks standard.
gprof.1: $(srcdir)/gprof.texi config.texi
	touch $@
	-$(TEXI2POD) $(MANCONF) -Dgprof < $(srcdir)/gprof.texi > gprof.pod
	-($(POD2MAN) gprof.pod | \
	       sed -e '/^.if n .na/d' > $@.T$$$$ && \
	       mv -f $@.T$$$$ $@) || \
	       (rm -f $@.T$$$$ && exit 1)
	rm -f gprof.pod

MAINTAINERCLEANFILES = gprof.info

# Automake 1.9 will only build info files in the objdir if they are
# mentioned in DISTCLEANFILES.  It doesn't have to be unconditional,
# though, so we use a bogus condition.
if GENINSRC_NEVER
DISTCLEANFILES = gprof.info
endif
