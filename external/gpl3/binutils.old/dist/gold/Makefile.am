# Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = po testsuite

tooldir = $(exec_prefix)/$(target_alias)

ACLOCAL_AMFLAGS = -I ../bfd -I ../config

AM_CFLAGS = $(WARN_CFLAGS) $(LFS_CFLAGS) $(RANDOM_SEED_CFLAGS)
AM_CXXFLAGS = $(WARN_CXXFLAGS) $(LFS_CFLAGS) $(RANDOM_SEED_CFLAGS)

AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/../include -I$(srcdir)/../elfcpp \
	-DLOCALEDIR="\"$(datadir)/locale\"" \
	-DBINDIR="\"$(bindir)\"" -DTOOLBINDIR="\"$(tooldir)/bin\"" \
	-DTOOLLIBDIR="\"$(tooldir)/lib\"" @INCINTL@

LIBIBERTY = ../libiberty/libiberty.a

if PLUGINS
LIBDL = -ldl
endif

if THREADS
THREADSLIB = -lpthread
endif

AM_YFLAGS = -d

# Automake 1.10+ disables lex and yacc output file regeneration if
# maintainer mode is disabled.  Avoid this.
am__skiplex =
am__skipyacc =

noinst_PROGRAMS = ld-new incremental-dump
noinst_LIBRARIES = libgold.a

CCFILES = \
	archive.cc \
	attributes.cc \
	binary.cc \
	common.cc \
	compressed_output.cc \
	copy-relocs.cc \
	cref.cc \
	defstd.cc \
	descriptors.cc \
	dirsearch.cc \
	dynobj.cc \
	dwarf_reader.cc \
	ehframe.cc \
	errors.cc \
	expression.cc \
	fileread.cc \
        gc.cc \
        gdb-index.cc \
	gold.cc \
	gold-threads.cc \
	icf.cc \
	incremental.cc \
	int_encoding.cc \
	layout.cc \
	mapfile.cc \
	merge.cc \
	nacl.cc \
	object.cc \
	options.cc \
	output.cc \
	parameters.cc \
	plugin.cc \
	readsyms.cc \
	reduced_debug_output.cc \
	reloc.cc \
	resolve.cc \
	script-sections.cc \
	script.cc \
	stringpool.cc \
	symtab.cc \
	target.cc \
	target-select.cc \
	timer.cc \
	version.cc \
	workqueue.cc \
	workqueue-threads.cc

HFILES = \
	arm-reloc-property.h \
	archive.h \
	attributes.h \
	binary.h \
	common.h \
	compressed_output.h \
	copy-relocs.h \
	cref.h \
	defstd.h \
	dirsearch.h \
	descriptors.h \
	dynobj.h \
	dwarf_reader.h \
	ehframe.h \
	errors.h \
	fileread.h \
	freebsd.h \
        gc.h \
        gdb-index.h \
	gold.h \
	gold-threads.h \
	icf.h \
	int_encoding.h \
	layout.h \
	mapfile.h \
	merge.h \
	nacl.h \
	object.h \
	options.h \
	output.h \
	parameters.h \
	plugin.h \
	readsyms.h \
	reduced_debug_output.h \
	reloc.h \
	reloc-types.h \
	script-c.h \
	script-sections.h \
	script.h \
	stringpool.h \
	symtab.h \
	target.h \
	target-reloc.h \
	target-select.h \
	timer.h \
	tls.h \
	token.h \
	workqueue.h \
	workqueue-internal.h

YFILES = \
	yyscript.y

DEFFILES = arm-reloc.def

EXTRA_DIST = yyscript.c yyscript.h

TARGETSOURCES = \
	i386.cc x86_64.cc sparc.cc powerpc.cc arm.cc arm-reloc-property.cc

ALL_TARGETOBJS = \
	i386.$(OBJEXT) x86_64.$(OBJEXT) sparc.$(OBJEXT) powerpc.$(OBJEXT) \
	arm.$(OBJEXT) arm-reloc-property.$(OBJEXT)

libgold_a_SOURCES = $(CCFILES) $(HFILES) $(YFILES) $(DEFFILES)
libgold_a_LIBADD = $(LIBOBJS)

sources_var = main.cc
deps_var = $(TARGETOBJS) libgold.a $(LIBIBERTY) $(LIBINTL_DEP)
ldadd_var = $(TARGETOBJS) libgold.a $(LIBIBERTY) $(GOLD_LDADD) $(LIBINTL) \
	 $(THREADSLIB) $(LIBDL)
ldflags_var = $(GOLD_LDFLAGS)

ld_new_SOURCES = $(sources_var)
ld_new_DEPENDENCIES = $(deps_var)
ld_new_LDADD = $(ldadd_var)
ld_new_LDFLAGS = $(ldflags_var)

EXTRA_ld_new_SOURCES = $(TARGETSOURCES)

incremental_dump_SOURCES = incremental-dump.cc
incremental_dump_DEPENDENCIES = $(TARGETOBJS) libgold.a $(LIBIBERTY) \
	$(LIBINTL_DEP)
incremental_dump_LDADD = $(TARGETOBJS) libgold.a $(LIBIBERTY) $(LIBINTL) \
	 $(THREADSLIB) $(LIBDL)

# Use an explicit dependency for the bison generated header file.
expression.$(OBJEXT): yyscript.h
script-sections.$(OBJEXT): yyscript.h
script.$(OBJEXT): yyscript.h

# We have to build libgold.a before we run the tests.
check: libgold.a

.PHONY: install-exec-local

install-exec-local: ld-new$(EXEEXT)
	$(mkinstalldirs) $(DESTDIR)$(bindir) $(DESTDIR)$(tooldir)/bin
	n=`echo $(installed_linker) | sed '$(transform)'`; \
	$(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(bindir)/$${n}$(EXEEXT); \
	if test "$(bindir)" != "$(tooldir)/bin"; then \
	  rm -f $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$${n}$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	fi; \
	if test "x$(install_as_default)" = "xyes"; then \
	  ld=`echo ld | sed '$(transform)'`; \
	  rm -f $(DESTDIR)$(bindir)/$${ld}$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$${n}$(EXEEXT) $(DESTDIR)$(bindir)/$${ld}$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(bindir)/$${ld}$(EXEEXT); \
	  if test "$(bindir)" != "$(tooldir)/bin"; then \
	    rm -f $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	    ln $(DESTDIR)$(bindir)/$${n}$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT) >/dev/null 2>/dev/null \
	    || $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	  fi; \
	fi

# We want install to imply install-info as per GNU standards, despite
# the cygnus option.
install-data-local: install-info

POTFILES= $(CCFILES) $(HFILES) $(TARGETSOURCES)

po/POTFILES.in: @MAINT@ Makefile
	for f in $(POTFILES); do echo $$f; done | LC_ALL=C sort > tmp \
	  && mv tmp $(srcdir)/po/POTFILES.in

# Bootstrap test support.  We use ld-new to build ld1, then use ld1 to
# build ld2.  ld1 and ld2 should be identical.  ld-new need not be
# identical to ld1, since it was linked with the host linker.

if GCC
if NATIVE_LINKER

gcctestdir1/ld: ld-new
	test -d gcctestdir1 || mkdir -p gcctestdir1
	rm -f gcctestdir1/ld
	(cd gcctestdir1 && $(LN_S) ../ld-new ld)

ld1_SOURCES = $(sources_var)
ld1_DEPENDENCIES = $(deps_var) gcctestdir1/ld
ld1_LDADD = $(ldadd_var)
ld1_LDFLAGS = -Bgcctestdir1/

gcctestdir2/ld: ld1
	test -d gcctestdir2 || mkdir -p gcctestdir2
	rm -f gcctestdir2/ld
	(cd gcctestdir2 && $(LN_S) ../ld1 ld)

ld2_SOURCES = $(sources_var)
ld2_DEPENDENCIES = $(deps_var) gcctestdir2/ld
ld2_LDADD = $(ldadd_var)
ld2_LDFLAGS = -Bgcctestdir2/

bootstrap-test: ld2
	rm -f $@
	echo "#!/bin/sh" > $@
	echo "cmp ld1 ld2" > $@
	chmod +x $@

libgold-1-r.o: gcctestdir1/ld libgold.a
	gcctestdir1/ld -o $@ -r --whole-archive libgold.a

ld1_r_SOURCES = $(sources_var)
ld1_r_DEPENDENCIES = libgold-1-r.o $(deps_var) gcctestdir1/ld
ld1_r_LDADD = libgold-1-r.o $(ldadd_var)
ld1_r_LDFLAGS = -Bgcctestdir1/

gcctestdir2-r/ld: ld1-r
	test -d gcctestdir2-r || mkdir -p gcctestdir2-r
	rm -f gcctestdir2-r/ld
	(cd gcctestdir2-r && $(LN_S) ../ld1-r ld)

libgold-2-r.o: gcctestdir2-r/ld libgold.a
	gcctestdir2-r/ld -o $@ -r --whole-archive libgold.a

ld2_r_SOURCES = $(sources_var)
ld2_r_DEPENDENCIES = libgold-2-r.o $(deps_var) gcctestdir2-r/ld
ld2_r_LDADD = libgold-2-r.o $(ldadd_var)
ld2_r_LDFLAGS = -Bgcctestdir2-r/

bootstrap-test-r: ld2-r
	rm -f $@
	echo "#!/bin/sh" > $@
	echo "cmp ld1-r ld2-r" > $@
	chmod +x $@

check_PROGRAMS = ld1 ld2 ld1-r ld2-r
TESTS = bootstrap-test bootstrap-test-r

endif
endif
