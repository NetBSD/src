## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

SUFFIXES = .m

INCLUDES = -D_GNU_SOURCE -DDEBUG -I../bfd -I$(srcdir)/../include -I$(srcdir)/../bfd

bin_PROGRAMS = gprof

gprof_SOURCES = basic_blocks.c call_graph.c cg_arcs.c cg_dfn.c \
	cg_print.c core.c gmon_io.c gprof.c hertz.c hist.c source.c \
	search_list.c symtab.c sym_ids.c utils.c \
	flat_bl.c bsd_callg_bl.c fsf_callg_bl.c \
	i386.c alpha.c vax.c tahoe.c sparc.c
gprof_DEPENDENCIES = ../bfd/libbfd.la ../libiberty/libiberty.a
gprof_LDADD = ../bfd/libbfd.la ../libiberty/libiberty.a

noinst_HEADERS = \
	basic_blocks.h call_graph.h cg_arcs.h cg_dfn.h cg_print.h \
	core.h gmon.h gmon_io.h gmon_out.h gprof.h hertz.h hist.h \
	search_list.h source.h sym_ids.h symtab.h utils.h

EXTRA_DIST = flat_bl.c bsd_callg_bl.c fsf_callg_bl.c bbconv.pl

BUILT_SOURCES = flat_bl.c bsd_callg_bl.c fsf_callg_bl.c

diststuff: $(BUILT_SOURCES) info

.m.c:
	awk -f $(srcdir)/gen-c-prog.awk > ./$*.c \
	    FUNCTION=`(echo $*|sed -e 's,.*/,,g' -e 's/_bl//')`_blurb \
	    FILE=$*.m $(srcdir)/$*.m

info_TEXINFOS = gprof.texi
man_MANS = gprof.1

# Dependencies.
$(OBJECTS): ../bfd/bfd.h call_graph.h cg_arcs.h cg_print.h \
	core.h gmon_io.h gmon_out.h gprof.h hertz.h hist.h \
	search_list.h source.h sym_ids.h symtab.h utils.h \
	$(srcdir)/../include/libiberty.h $(srcdir)/../bfd/sysdep.h \
	gconfig.h ../bfd/config.h
basic_blocks.o: basic_blocks.c
bsd_call_bl.o: bsd_call_bl.c
call_graph.o: call_graph.c
cg_arcs.o: cg_arcs.c
cg_dfn.o: cg_dfn.c
cg_print.o: cg_print.c
core.o: core.c
flat_bl.o: flat_bl.c
fsf_callg_bl.o: fsf_callg_bl.c
gmon_io.o: gmon_io.c
gprof.o: gprof.c
hertz.o: hertz.c
hist.o: hist.c
search_list.o: search_list.c
source.o: source.c
symtab.o: symtab.c
sym_ids.o: sym_ids.c
utils.o: utils.c
i386.o: i386.c
alpha.o: alpha.c
vax.o: vax.c
tahoe.o: tahoe.c
sparc.o: sparc.c
