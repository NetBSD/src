## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = dejagnu no-dist foreign
ACLOCAL_AMFLAGS = -I .. -I ../config -I ../bfd

SUBDIRS = doc po

tooldir = $(exec_prefix)/$(target_alias)

## These aren't set by automake, because they appear in
## bfd/acinclude.m4, which is included by binutils/acinclude.m4, and
## thus is not seen by automake.
CC_FOR_BUILD = @CC_FOR_BUILD@
EXEEXT_FOR_BUILD = @EXEEXT_FOR_BUILD@

YACC = `if [ -f ../bison/bison ]; then echo ../bison/bison -y -L$(srcdir)/../bison/; else echo @YACC@; fi`
YFLAGS = -d
LEX = `if [ -f ../flex/flex ]; then echo ../flex/flex; else echo @LEX@; fi`

# Automake 1.10+ disables lex and yacc output file regeneration if
# maintainer mode is disabled.  Avoid this.
am__skiplex =
am__skipyacc =

WARN_CFLAGS = @WARN_CFLAGS@
NO_WERROR = @NO_WERROR@
AM_CFLAGS = $(WARN_CFLAGS)
LIBICONV = @LIBICONV@

# these two are almost the same program
AR_PROG=ar
RANLIB_PROG=ranlib

# objcopy and strip should be the same program
OBJCOPY_PROG=objcopy
STRIP_PROG=strip-new

STRINGS_PROG=strings

READELF_PROG=readelf

ELFEDIT_PROG=elfedit

# These should all be the same program too.
SIZE_PROG=size
NM_PROG=nm-new
OBJDUMP_PROG=objdump

# This is the demangler, as a standalone program.
# Note: This one is used as the installed name too, unlike the above.
DEMANGLER_PROG=cxxfilt

ADDR2LINE_PROG=addr2line

NLMCONV_PROG=nlmconv
DLLTOOL_PROG=dlltool
WINDRES_PROG=windres
WINDMC_PROG=windmc
DLLWRAP_PROG=dllwrap

SRCONV_PROG=srconv$(EXEEXT) sysdump$(EXEEXT) coffdump$(EXEEXT)

bin_PROGRAMS = $(SIZE_PROG) $(OBJDUMP_PROG) $(AR_PROG) $(STRINGS_PROG) $(RANLIB_PROG) $(OBJCOPY_PROG) @BUILD_NLMCONV@ @BUILD_SRCONV@ @BUILD_DLLTOOL@ @BUILD_WINDRES@ @BUILD_WINDMC@ $(ADDR2LINE_PROG) $(READELF_PROG) $(ELFEDIT_PROG) @BUILD_DLLWRAP@

bin_SCRIPTS = @BUILD_INSTALL_MISC@
EXTRA_SCRIPTS = embedspu

## We need a special rule to install the programs which are built with
## -new, and to rename cxxfilt to c++filt.
RENAMED_PROGS = $(NM_PROG) $(STRIP_PROG) $(DEMANGLER_PROG)
noinst_PROGRAMS = $(RENAMED_PROGS) @BUILD_MISC@

EXTRA_PROGRAMS = $(NLMCONV_PROG) srconv sysdump coffdump $(DLLTOOL_PROG) $(WINDRES_PROG) $(WINDMC_PROG) $(DLLWRAP_PROG)

# Stuff that goes in tooldir/ if appropriate.
TOOL_PROGS = nm-new strip-new ar ranlib dlltool objdump objcopy

BASEDIR = $(srcdir)/..
BFDDIR = $(BASEDIR)/bfd
INCDIR	= $(BASEDIR)/include

AM_CPPFLAGS = -I. -I$(srcdir) -I../bfd -I$(BFDDIR) -I$(INCDIR) \
	 @HDEFINES@ \
	 @INCINTL@ \
	 -DLOCALEDIR="\"$(datadir)/locale\"" \
	 -Dbin_dummy_emulation=$(EMULATION_VECTOR)

HFILES = \
	arsup.h binemul.h bucomm.h budbg.h \
	coffgrok.h debug.h dlltool.h dwarf.h elfcomm.h nlmconv.h \
	objdump.h sysdep.h unwind-ia64.h windres.h winduni.h windint.h \
	windmc.h

GENERATED_HFILES = arparse.h sysroff.h sysinfo.h defparse.h rcparse.h mcparse.h
BUILT_SOURCES = $(GENERATED_HFILES)

CFILES = \
	addr2line.c ar.c arsup.c bin2c.c binemul.c bucomm.c \
	coffdump.c coffgrok.c cxxfilt.c \
	dwarf.c debug.c dlltool.c dllwrap.c \
	elfcomm.c emul_aix.c emul_vanilla.c filemode.c \
	ieee.c is-ranlib.c is-strip.c maybe-ranlib.c maybe-strip.c \
	nlmconv.c nm.c not-ranlib.c not-strip.c \
	objcopy.c objdump.c prdbg.c \
	od-xcoff.c od-macho.c \
	rclex.c rdcoff.c rddbg.c readelf.c rename.c \
	resbin.c rescoff.c resrc.c resres.c \
	size.c srconv.c stabs.c strings.c sysdump.c \
	syslex_wrap.c unwind-ia64.c elfedit.c version.c \
	windres.c winduni.c wrstabs.c \
	windmc.c mclex.c

GENERATED_CFILES = \
	arparse.c arlex.c sysroff.c sysinfo.c syslex.c \
	defparse.c deflex.c nlmheader.c rcparse.c mcparse.c

DEBUG_SRCS = rddbg.c debug.c stabs.c ieee.c rdcoff.c
WRITE_DEBUG_SRCS = $(DEBUG_SRCS) wrstabs.c

# Extra object files for objdump
OBJDUMP_PRIVATE_OFILES = @OBJDUMP_PRIVATE_OFILES@

# Code shared by all the binutils.
BULIBS = bucomm.c version.c filemode.c

# Code shared by the ELF related programs.
ELFLIBS = elfcomm.c

BFDLIB = ../bfd/libbfd.la

OPCODES = ../opcodes/libopcodes.la

LIBIBERTY = ../libiberty/libiberty.a

POTFILES = $(CFILES) $(DEBUG_SRCS) $(HFILES)
po/POTFILES.in: @MAINT@ Makefile
	for f in $(POTFILES); do echo $$f; done | LC_ALL=C sort > tmp \
	  && mv tmp $(srcdir)/po/POTFILES.in

EXPECT = expect
RUNTEST = runtest

CC_FOR_TARGET = ` \
  if [ -f $$r/../gcc/xgcc ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/xgcc -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/xgcc -B$$r/../gcc/; \
    fi; \
  else \
    if [ "@host@" = "@target@" ] ; then \
      echo $(CC); \
    else \
      echo gcc | sed '$(transform)'; \
    fi; \
  fi`

check-DEJAGNU: site.exp
	srcdir=`cd $(srcdir) && pwd`; export srcdir; \
	r=`pwd`; export r; \
	LC_ALL=C; export LC_ALL; \
	EXPECT=$(EXPECT); export EXPECT; \
	runtest=$(RUNTEST); \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  CC_FOR_TARGET="$(CC_FOR_TARGET)" CFLAGS_FOR_TARGET="$(CFLAGS)" \
		$$runtest --tool $(DEJATOOL) --srcdir $${srcdir}/testsuite \
			$(RUNTESTFLAGS); \
	else echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

installcheck-local:
	/bin/sh $(srcdir)/sanity.sh $(bindir)

# There's no global DEPENDENCIES.  So, we must explicitly list everything
# which depends on libintl, since we don't know whether LIBINTL_DEP will be
# non-empty until configure time.  Ugh!
size_DEPENDENCIES =      $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
objdump_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB) $(OPCODES) $(OBJDUMP_PRIVATE_OFILES)
nm_new_DEPENDENCIES =    $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
ar_DEPENDENCIES =        $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
strings_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
strip_new_DEPENDENCIES = $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
ranlib_DEPENDENCIES =    $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
cxxfilt_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
objcopy_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
nlmconv_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
srconv_DEPENDENCIES =    $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
sysdump_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
coffdump_DEPENDENCIES =  $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
dlltool_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
windres_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
windmc_DEPENDENCIES =    $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
addr2line_DEPENDENCIES = $(LIBINTL_DEP) $(LIBIBERTY) $(BFDLIB)
readelf_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY)
elfedit_DEPENDENCIES = $(LIBINTL_DEP) $(LIBIBERTY)
dllwrap_DEPENDENCIES =   $(LIBINTL_DEP) $(LIBIBERTY)

LDADD = $(BFDLIB) $(LIBIBERTY) $(LIBINTL)

size_SOURCES = size.c $(BULIBS)

objcopy_SOURCES = objcopy.c not-strip.c rename.c $(WRITE_DEBUG_SRCS) $(BULIBS)

strings_SOURCES = strings.c $(BULIBS)

readelf_SOURCES = readelf.c version.c unwind-ia64.c dwarf.c $(ELFLIBS)
readelf_LDADD   = $(LIBINTL) $(LIBIBERTY)

elfedit_SOURCES = elfedit.c version.c $(ELFLIBS)
elfedit_LDADD = $(LIBINTL) $(LIBIBERTY)

strip_new_SOURCES = objcopy.c is-strip.c rename.c $(WRITE_DEBUG_SRCS) $(BULIBS)

nm_new_SOURCES = nm.c $(BULIBS)

objdump_SOURCES = objdump.c dwarf.c prdbg.c $(DEBUG_SRCS) $(BULIBS) $(ELFLIBS)
EXTRA_objdump_SOURCES = od-xcoff.c
objdump_LDADD = $(OBJDUMP_PRIVATE_OFILES) $(OPCODES) $(BFDLIB) $(LIBIBERTY) $(LIBINTL)

objdump.@OBJEXT@:objdump.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $(OBJDUMP_DEFS) $(srcdir)/objdump.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='objdump.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(OBJDUMP_DEFS) $(srcdir)/objdump.c
endif

cxxfilt_SOURCES = cxxfilt.c $(BULIBS)

ar_SOURCES = arparse.y arlex.l ar.c not-ranlib.c arsup.c rename.c binemul.c \
	emul_$(EMULATION).c $(BULIBS)
EXTRA_ar_SOURCES = $(CFILES)
ar_LDADD = $(BFDLIB) $(LIBIBERTY) @LEXLIB@ $(LIBINTL)

ranlib_SOURCES = ar.c is-ranlib.c arparse.y arlex.l arsup.c rename.c \
	binemul.c emul_$(EMULATION).c $(BULIBS)
ranlib_LDADD = $(BFDLIB) $(LIBIBERTY) @LEXLIB@ $(LIBINTL)

addr2line_SOURCES = addr2line.c $(BULIBS)

# The following is commented out for the conversion to automake.
# This rule creates a single binary that switches between ar and ranlib
# by looking at argv[0].  Use this kludge to save some disk space.
# However, you have to install things by hand.
# (That is after 'make install', replace the installed ranlib by a link to ar.)
# Alternatively, you can install ranlib.sh as ranlib.
# ar_with_ranlib: $(ADDL_DEPS) ar.o maybe-ranlib.o
#	$(HLDENV) $(CC) $(HLDFLAGS) $(CFLAGS) $(LDFLAGS) -o $(AR_PROG) ar.o maybe-ranlib.o $(ADDL_LIBS) $(EXTRALIBS)
#	-rm -f $(RANLIB_PROG)
#	-ln $(AR_PROG) $(RANLIB_PROG)
#
# objcopy and strip in one binary that uses argv[0] to decide its action.
#
#objcopy_with_strip: $(ADDL_DEPS) objcopy.o maybe-strip.o
#	$(HLDENV) $(CC) $(HLDFLAGS) $(CFLAGS) $(LDFLAGS) -o $(OBJCOPY_PROG) objcopy.o maybe-strip.o $(ADDL_LIBS) $(EXTRALIBS)
#	-rm -f $(STRIP_PROG)
#	-ln $(OBJCOPY_PROG) $(STRIP_PROG)

sysroff.c: sysinfo$(EXEEXT_FOR_BUILD) sysroff.info
	./sysinfo$(EXEEXT_FOR_BUILD) -c <$(srcdir)/sysroff.info >sysroff.c
	./sysinfo$(EXEEXT_FOR_BUILD) -i <$(srcdir)/sysroff.info >>sysroff.c
	./sysinfo$(EXEEXT_FOR_BUILD) -g <$(srcdir)/sysroff.info >>sysroff.c

sysroff.h: sysinfo$(EXEEXT_FOR_BUILD) sysroff.info
	./sysinfo$(EXEEXT_FOR_BUILD) -d <$(srcdir)/sysroff.info >sysroff.h

sysinfo$(EXEEXT_FOR_BUILD): sysinfo.@OBJEXT@ syslex_wrap.@OBJEXT@
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) -o $@ sysinfo.@OBJEXT@ syslex_wrap.@OBJEXT@

syslex_wrap.@OBJEXT@: syslex_wrap.c syslex.c sysinfo.h config.h
	$(CC_FOR_BUILD) -c -I. -I$(srcdir) $(AM_CFLAGS) $(CFLAGS_FOR_BUILD) $(NO_WERROR) $(srcdir)/syslex_wrap.c

sysinfo.@OBJEXT@: sysinfo.c
	if [ -r sysinfo.c ]; then \
	  $(CC_FOR_BUILD) -c -I. $(AM_CFLAGS) $(CFLAGS_FOR_BUILD) $(NO_WERROR) sysinfo.c ; \
	else \
	  $(CC_FOR_BUILD) -c -I. $(AM_CFLAGS) $(CFLAGS_FOR_BUILD) $(NO_WERROR) $(srcdir)/sysinfo.c ; \
	fi

bin2c$(EXEEXT_FOR_BUILD): bin2c.c
	$(CC_FOR_BUILD) -o $@ $(AM_CPPFLAGS) $(AM_CFLAGS) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(srcdir)/bin2c.c

embedspu: embedspu.sh Makefile
	awk '/^program_transform_name=/ {print "program_transform_name=\"$(program_transform_name)\""; next} {print}' < $< > $@
	chmod a+x $@

# We need these for parallel make.
sysinfo.h: sysinfo.c

# Disable -Werror, if it has been enabled, since old versions of bison/
# yacc will produce working code which contain compile time warnings.
arparse.@OBJEXT@: arparse.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f arparse.c || echo $(srcdir)/`arparse.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='arparse.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f arparse.c || echo $(srcdir)/`arparse.c $(NO_WERROR)
endif

arlex.@OBJEXT@: arlex.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f arlex.c || echo $(srcdir)/`arlex.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='arlex.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f arlex.c || echo $(srcdir)/`arlex.c $(NO_WERROR)
endif

sysroff.@OBJEXT@: sysroff.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f sysroff.c || echo $(srcdir)/`sysroff.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='sysroff.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f sysroff.c || echo $(srcdir)/`sysroff.c $(NO_WERROR)
endif

defparse.@OBJEXT@: defparse.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f defparse.c || echo $(srcdir)/`defparse.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='defparse.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f defparse.c || echo $(srcdir)/`defparse.c $(NO_WERROR)
endif

deflex.@OBJEXT@: deflex.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f deflex.c || echo $(srcdir)/`deflex.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='deflex.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f deflex.c || echo $(srcdir)/`deflex.c $(NO_WERROR)
endif

nlmheader.@OBJEXT@: nlmheader.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f nlmheader.c || echo $(srcdir)/`nlmheader.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='nlmheader.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f nlmheader.c || echo $(srcdir)/`nlmheader.c $(NO_WERROR)
endif

rcparse.@OBJEXT@: rcparse.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f rcparse.c || echo $(srcdir)/`rcparse.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='rcparse.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f rcparse.c || echo $(srcdir)/`rcparse.c $(NO_WERROR)
endif

mcparse.@OBJEXT@: mcparse.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f mcparse.c || echo $(srcdir)/`mcparse.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='mcparse.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f mcparse.c || echo $(srcdir)/`mcparse.c $(NO_WERROR)
endif

rclex.@OBJEXT@: rclex.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $(srcdir)/rclex.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='rclex.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(srcdir)/rclex.c $(NO_WERROR)
endif

mclex.@OBJEXT@: mclex.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $(srcdir)/mclex.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='mclex.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(srcdir)/mclex.c $(NO_WERROR)
endif

srconv_SOURCES = srconv.c coffgrok.c $(BULIBS)
srconv.@OBJEXT@: sysroff.c

dlltool_SOURCES = dlltool.c defparse.y deflex.l $(BULIBS)
dlltool_LDADD = $(BFDLIB) $(LIBIBERTY) @LEXLIB@ $(LIBINTL)

dlltool.@OBJEXT@:
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@  $(DLLTOOL_DEFS) $(srcdir)/dlltool.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='dlltool.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(DLLTOOL_DEFS) $(srcdir)/dlltool.c
endif

rescoff.@OBJEXT@:
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@  $(DLLTOOL_DEFS) $(srcdir)/rescoff.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='rescoff.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(DLLTOOL_DEFS) $(srcdir)/rescoff.c
endif

coffdump_SOURCES = coffdump.c coffgrok.c $(BULIBS)

sysdump_SOURCES = sysdump.c $(BULIBS)
sysdump.@OBJEXT@: sysroff.c

# coff/sym.h and coff/ecoff.h won't be found by the automatic dependency
# scripts, since they are only included conditionally.
nlmconv.@OBJEXT@: nlmconv.c
if am__fastdepCC
	ldname=`echo ld | sed '$(transform)'`; \
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	  -DLD_NAME="\"$${ldname}\"" @NLMCONV_DEFS@ $(srcdir)/nlmconv.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='nlmconv.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	ldname=`echo ld | sed '$(transform)'`; \
	$(COMPILE) -c -DLD_NAME="\"$${ldname}\"" @NLMCONV_DEFS@ $(srcdir)/nlmconv.c
endif

nlmconv_SOURCES = nlmconv.c nlmheader.y $(BULIBS)

windres_SOURCES = windres.c resrc.c rescoff.c resbin.c rcparse.y rclex.c \
	winduni.c resres.c $(BULIBS)
windres_LDADD = $(BFDLIB) $(LIBIBERTY) @LEXLIB@ $(LIBINTL) $(LIBICONV)

windmc_SOURCES = windmc.c mcparse.y mclex.c \
	winduni.c $(BULIBS)
windmc_LDADD = $(BFDLIB) $(LIBIBERTY) @LEXLIB@ $(LIBINTL) $(LIBICONV)

dllwrap_SOURCES = dllwrap.c version.c
dllwrap_LDADD = $(LIBIBERTY) $(LIBINTL)


EXTRA_DIST = arparse.c arparse.h arlex.c nlmheader.c sysinfo.c sysinfo.h \
	syslex.c deflex.c defparse.h defparse.c rcparse.h rcparse.c \
	mcparse.h mcparse.c embedspu.sh

diststuff: $(EXTRA_DIST) info
all: info

# We extract version from bfd/configure.in, make sure to rerun configure
# when BFD's version changes.
CONFIG_STATUS_DEPENDENCIES = $(BFDDIR)/configure.in

DISTCLEANFILES = sysroff.c sysroff.h site.exp site.bak embedspu

MOSTLYCLEANFILES = sysinfo$(EXEEXT_FOR_BUILD) bin2c$(EXEEXT_FOR_BUILD) \
  binutils.log binutils.sum abcdefgh*
mostlyclean-local:
	-rm -rf tmpdir

.PHONY: install-exec-local

install-exec-local: install-binPROGRAMS $(bin_PROGRAMS) $(noinst_PROGRAMS)
	@list='$(RENAMED_PROGS)'; for p in $$list; do \
	  if test -f $$p$(EXEEXT); then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p$(EXEEXT) $(bindir)/`echo $$p|sed -e 's/-new//' -e 's/cxxfilt/$(DEMANGLER_NAME)/'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p$(EXEEXT) $(DESTDIR)$(bindir)/`echo $$p|sed -e 's/-new//' -e 's/cxxfilt/$(DEMANGLER_NAME)/'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/bin
	for i in $(TOOL_PROGS); do \
	  if [ -f $$i$(EXEEXT) ]; then \
	    j=`echo $$i | sed -e 's/-new//'`; \
	    k=`echo $$j | sed '$(transform)'`; \
	    if [ "$(bindir)/$$k" != "$(tooldir)/bin/$$j" ]; then \
	      rm -f $(DESTDIR)$(tooldir)/bin/$$j$(EXEEXT); \
	      ln $(DESTDIR)$(bindir)/$$k$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$$j$(EXEEXT) >/dev/null 2>/dev/null \
		|| $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) $$i$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$$j$(EXEEXT); \
	    fi; \
	  else true; \
	  fi; \
	done
