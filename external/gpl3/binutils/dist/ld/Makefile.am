## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = dejagnu no-texinfo.tex no-dist foreign
ACLOCAL_AMFLAGS = -I .. -I ../config -I ../bfd
TEXINFO_TEX = ../texinfo/texinfo.tex

SUBDIRS = po

tooldir = $(exec_prefix)/$(target_alias)

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

# Conditionally enable the plugin interface.
if ENABLE_PLUGINS
PLUGIN_C = plugin.c
PLUGIN_H = plugin.h
PLUGIN_OBJECT = plugin.@OBJEXT@
PLUGIN_CFLAGS = -DENABLE_PLUGINS
else
PLUGIN_C =
PLUGIN_H =
PLUGIN_OBJECT =
PLUGIN_CFLAGS =
endif

# We put the scripts in the directory $(scriptdir)/ldscripts.
# We can't put the scripts in $(datadir) because the SEARCH_DIR
# directives need to be different for native and cross linkers.
scriptdir = $(tooldir)/lib

EMUL = @EMUL@
EMULATION_OFILES = @EMULATION_OFILES@
EMUL_EXTRA_OFILES = @EMUL_EXTRA_OFILES@

# Search path to override the default search path for -lfoo libraries.
# If LIB_PATH is empty, the ones in the script (if any) are left alone.
# (The default is usually /lib:/usr/lib:/usr/local/lib, unless building
# a cross-linker, in which case the default is empty.  See genscripts.sh.)
# Otherwise, they are replaced with the ones given in LIB_PATH,
# which may have the form: LIB_PATH=/lib:/usr/local/lib.  This can be set
# when the linker is configured via the --with-lib-path configure switch.
LIB_PATH = @LIB_PATH@

BASEDIR = $(srcdir)/..
BFDDIR = $(BASEDIR)/bfd
INCDIR	= $(BASEDIR)/include

# What version of the manual to build
DOCVER = gen

# Options to extract the man page from ld.texinfo
MANCONF = -Dman

TEXI2POD = perl $(BASEDIR)/etc/texi2pod.pl $(AM_MAKEINFOFLAGS)

POD2MAN = pod2man --center="GNU Development Tools" \
	--release="binutils-$(VERSION)" --section=1

#stuff for self hosting (can be overridden in config file).
HOSTING_CRT0 = @HOSTING_CRT0@
HOSTING_LIBS = @HOSTING_LIBS@
HOSTING_EMU = -m $(EMUL)

# Setup the testing framework, if you have one
EXPECT = expect
RUNTEST = runtest
RUNTESTFLAGS =

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

CXX_FOR_TARGET = ` \
  if [ -f $$r/../gcc/g++ ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/g++ -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/g++ -B$$r/../gcc/; \
    fi; \
  else \
    if [ "@host@" = "@target@" ] ; then \
      echo $(CXX); \
    else \
      echo g++ | sed '$(transform)'; \
    fi; \
  fi`

transform = s/^ld-new$$/$(installed_linker)/;@program_transform_name@
bin_PROGRAMS = ld-new
info_TEXINFOS = ld.texinfo
ld_TEXINFOS = configdoc.texi
noinst_TEXINFOS = ldint.texinfo
man_MANS = ld.1

AM_MAKEINFOFLAGS = -I $(srcdir) -I $(BFDDIR)/doc -I ../bfd/doc \
		   -I $(top_srcdir)/../libiberty
TEXI2DVI = texi2dvi -I $(srcdir) -I $(BFDDIR)/doc -I ../bfd/doc \
		    -I $(top_srcdir)/../libiberty

AM_CPPFLAGS = -I. -I$(srcdir) -I../bfd -I$(BFDDIR) -I$(INCDIR) \
	@INCINTL@ $(HDEFINES) $(CFLAGS) $(PLUGIN_CFLAGS) \
	-DLOCALEDIR="\"$(datadir)/locale\""

BFDLIB = ../bfd/libbfd.la
LIBIBERTY = ../libiberty/libiberty.a

ALL_EMULATION_SOURCES = \
	eaix5ppc.c \
	eaix5rs6.c \
	eaixppc.c \
	eaixrs6.c \
	ealpha.c \
	ealphavms.c \
	earcelf.c \
	earm_epoc_pe.c \
	earm_wince_pe.c \
	earmaoutb.c \
	earmaoutl.c \
	earmcoff.c \
	earmelf.c \
	earmelf_fbsd.c \
	earmelf_linux.c \
	earmelf_linux_eabi.c \
	earmelf_nacl.c \
	earmelf_nbsd.c \
	earmelf_nbsd_eabi.c \
	earmelf_nbsd_eabihf.c \
	earmelf_vxworks.c \
	earmelfb.c \
	earmelfb_linux.c \
	earmelfb_linux_eabi.c \
	earmelfb_nacl.c \
	earmelfb_nbsd.c \
	earmelfb_nbsd_eabi.c \
	earmelfb_nbsd_eabihf.c \
	earmnbsd.c \
	earmnto.c \
	earmpe.c \
	earmsymbian.c \
	eavr1.c \
	eavr2.c \
	eavr25.c \
	eavr3.c \
	eavr31.c \
	eavr35.c \
	eavr4.c \
	eavr5.c \
	eavr51.c \
	eavr6.c \
	eavrxmega1.o \
	eavrxmega2.o \
	eavrxmega3.o \
	eavrxmega4.o \
	eavrxmega5.o \
	eavrxmega6.o \
	eavrxmega7.o \
	ecoff_i860.c \
	ecoff_sparc.c \
	ecrisaout.c \
	ecriself.c \
	ecrislinux.c \
	ed10velf.c \
	ed30v_e.c \
	ed30v_o.c \
	ed30velf.c \
	edelta68.c \
	eelf32_dlx.c \
	eelf32_i860.c \
	eelf32_i960.c \
	eelf32_sparc.c \
	eelf32_sparc_sol2.c \
	eelf32_sparc_vxworks.c \
	eelf32_spu.c \
	eelf32_tic6x_be.c \
	eelf32_tic6x_le.c \
	eelf32_tic6x_linux_be.c \
	eelf32_tic6x_linux_le.c \
	eelf32_tic6x_elf_be.c \
	eelf32_tic6x_elf_le.c \
	eelf32am33lin.c \
	eelf32b4300.c \
	eelf32bfin.c \
	eelf32bfinfd.c \
	eelf32bmip.c \
	eelf32bmipn32.c \
	eelf32bsmip.c \
	eelf32btsmip.c \
	eelf32btsmip_fbsd.c \
	eelf32btsmipn32.c \
	eelf32btsmipn32_fbsd.c \
	eelf32cr16.c \
	eelf32cr16c.c \
	eelf32crx.c \
	eelf32ebmip.c \
	eelf32ebmipvxworks.c \
	eelf32elmip.c \
	eelf32elmipvxworks.c \
	eelf32epiphany.c \
	eelf32fr30.c \
	eelf32frv.c \
	eelf32frvfd.c \
	eelf32i370.c \
	eelf32ip2k.c \
	eelf32iq10.c \
	eelf32iq2000.c \
	eelf32l4300.c \
	eelf32lm32.c \
	eelf32lm32fd.c \
	eelf32lmip.c \
	eelf32lppc.c \
	eelf32lppcnto.c \
	eelf32lppcsim.c \
	eelf32lriscv.c \
	eelf32lsmip.c \
	eelf32ltsmip.c \
	eelf32ltsmip_fbsd.c \
	eelf32ltsmipn32.c \
	eelf32ltsmipn32_fbsd.c \
	eelf32m32c.c \
	eelf32mb_linux.c \
	eelf32mcore.c \
	eelf32mep.c \
	eelf32microblaze.c \
	eelf32mipswindiss.c \
	eelf32moxie.c \
	eelf32mt.c \
	eelf32or1k.c \
	eelf32or1k_linux.c \
	eelf32or1k_nbsd.c \
	eelf32ppc.c \
	eelf32ppc_fbsd.c \
	eelf32ppc_nbsd.c \
	eelf32ppclinux.c \
	eelf32ppcnto.c \
	eelf32ppcsim.c \
	eelf32ppcvxworks.c \
	eelf32ppcwindiss.c \
	eelf32rl78.c \
	eelf32rx.c \
	eelf32tilegx.c \
	eelf32tilegx_be.c \
	eelf32tilepro.c \
	eelf32vax.c \
	eelf32xc16x.c \
	eelf32xc16xl.c \
	eelf32xc16xs.c \
	eelf32xstormy16.c \
	eelf32xtensa.c \
	eelf_i386.c \
	eelf_i386_be.c \
	eelf_i386_chaos.c \
	eelf_i386_fbsd.c \
	eelf_i386_ldso.c \
	eelf_i386_nacl.c \
	eelf_i386_sol2.c \
	eelf_i386_vxworks.c \
	eelf_s390.c \
	egld960.c \
	egld960coff.c \
	eh8300.c \
	eh8300elf.c \
	eh8300h.c \
	eh8300helf.c \
	eh8300hn.c \
	eh8300hnelf.c \
	eh8300s.c \
	eh8300self.c \
	eh8300sn.c \
	eh8300snelf.c \
	eh8300sx.c \
	eh8300sxelf.c \
	eh8300sxn.c \
	eh8300sxnelf.c \
	eh8500.c \
	eh8500b.c \
	eh8500c.c \
	eh8500m.c \
	eh8500s.c \
	ehp300bsd.c \
	ehp3hpux.c \
	ehppaelf.c \
	ehppalinux.c \
	ehppanbsd.c \
	ehppaobsd.c \
	ei386aout.c \
	ei386beos.c \
	ei386bsd.c \
	ei386coff.c \
	ei386go32.c \
	ei386linux.c \
	ei386lynx.c \
	ei386mach.c \
	ei386moss.c \
	ei386msdos.c \
	ei386nbsd.c \
	ei386nto.c \
	ei386nw.c \
	ei386pe.c \
	ei386pe_posix.c \
	ei386pep.c \
	elnk960.c \
	em32relf.c \
	em32relf_linux.c \
	em32rlelf.c \
	em32rlelf_linux.c \
	em68hc11elf.c \
	em68hc11elfb.c \
	em68hc12elf.c \
	em68hc12elfb.c \
	em68k4knbsd.c \
	em68kaout.c \
	em68kaux.c \
	em68kcoff.c \
	em68kelf.c \
	em68kelfnbsd.c \
	em68klinux.c \
	em68knbsd.c \
	em68kpsos.c \
	em88kbcs.c \
	emcorepe.c \
	emipsbig.c \
	emipsbsd.c \
	emipsidt.c \
	emipsidtl.c \
	emipslit.c \
	emipslnews.c \
	emipspe.c \
	emn10200.c \
	emn10300.c \
	emsp430x110.c \
	emsp430x1101.c \
	emsp430x1111.c \
	emsp430x112.c \
	emsp430x1121.c \
	emsp430x1122.c \
	emsp430x1132.c \
	emsp430x122.c \
	emsp430x1222.c \
	emsp430x123.c \
	emsp430x1232.c \
	emsp430x133.c \
	emsp430x1331.c \
	emsp430x135.c \
	emsp430x1351.c \
	emsp430x147.c \
	emsp430x148.c \
	emsp430x149.c \
	emsp430x155.c \
	emsp430x156.c \
	emsp430x157.c \
	emsp430x1610.c \
	emsp430x1611.c \
	emsp430x1612.c \
	emsp430x167.c \
	emsp430x168.c \
	emsp430x169.c \
	emsp430x2101.c \
	emsp430x2111.c \
	emsp430x2121.c \
	emsp430x2131.c \
	emsp430x311.c \
	emsp430x312.c \
	emsp430x313.c \
	emsp430x314.c \
	emsp430x315.c \
	emsp430x323.c \
	emsp430x325.c \
	emsp430x336.c \
	emsp430x337.c \
	emsp430x412.c \
	emsp430x413.c \
	emsp430x415.c \
	emsp430x417.c \
	emsp430x435.c \
	emsp430x436.c \
	emsp430x437.c \
	emsp430x447.c \
	emsp430x448.c \
	emsp430x449.c \
	emsp430xE423.c \
	emsp430xE425.c \
	emsp430xE427.c \
	emsp430xG437.c \
	emsp430xG438.c \
	emsp430xG439.c \
	emsp430xW423.c \
	emsp430xW425.c \
	emsp430xW427.c \
	enews.c \
	ens32knbsd.c \
	epc532macha.c \
	epdp11.c \
	epjelf.c \
	epjlelf.c \
	eppclynx.c \
	eppcmacos.c \
	eppcnw.c \
	eppcpe.c \
	eriscix.c \
	escore3_elf.c \
	escore7_elf.c \
	esh.c \
	eshelf.c \
	eshelf32.c \
	eshelf32_linux.c \
	eshelf32_nbsd.c \
	eshelf_fd.c \
	eshelf_linux.c \
	eshelf_nbsd.c \
	eshelf_nto.c \
	eshelf_uclinux.c \
	eshelf_vxworks.c \
	eshl.c \
	eshlelf.c \
	eshlelf32.c \
	eshlelf32_linux.c \
	eshlelf32_nbsd.c \
	eshlelf_fd.c \
	eshlelf_linux.c \
	eshlelf_nbsd.c \
	eshlelf_nto.c \
	eshlelf_vxworks.c \
	eshlsymbian.c \
	eshpe.c \
	esparcaout.c \
	esparclinux.c \
	esparcnbsd.c \
	est2000.c \
	esun3.c \
	esun4.c \
	etic30aout.c \
	etic30coff.c \
	etic3xcoff.c \
	etic3xcoff_onchip.c \
	etic4xcoff.c \
	etic54xcoff.c \
	etic80coff.c \
	ev850.c \
	evanilla.c \
	evax.c \
	evaxnbsd.c \
	evsta.c \
	ew65.c \
	exgateelf.c \
	ez80.c \
	ez8001.c \
	ez8002.c

ALL_EMULATIONS = $(ALL_EMULATION_SOURCES:.c=.@OBJEXT@)

ALL_64_EMULATION_SOURCES = \
	eaarch64elf.c \
	eaarch64elfb.c \
	eaarch64linux.c \
	eaarch64linuxb.c \
	eaarch64nbsd.c \
	eaarch64nbsdb.c \
	eelf32_x86_64.c \
	eelf32_x86_64_nacl.c \
	eelf64_aix.c \
	eelf64_ia64.c \
	eelf64_ia64_fbsd.c \
	eelf64_ia64_vms.c \
	eelf64_s390.c \
	eelf64_sparc.c \
	eelf64_sparc_fbsd.c \
	eelf64_sparc_sol2.c \
	eelf64alpha.c \
	eelf64alpha_fbsd.c \
	eelf64alpha_nbsd.c \
	eelf64bmip.c \
	eelf64btsmip.c \
	eelf64btsmip_fbsd.c \
	eelf64hppa.c \
	eelf64lppc.c \
	eelf64lriscv.c \
	eelf64ltsmip.c \
	eelf64ltsmip_fbsd.c \
	eelf64mmix.c \
	eelf64ppc.c \
	eelf64ppc_fbsd.c \
	eelf64tilegx.c \
	eelf64tilegx_be.c \
	eelf_l1om.c \
	eelf_l1om_fbsd.c \
	eelf_k1om.c \
	eelf_k1om_fbsd.c \
	eelf_x86_64.c \
	eelf_x86_64_fbsd.c \
	eelf_x86_64_nacl.c \
	eelf_x86_64_sol2.c \
	ehppa64linux.c \
	emmo.c \
	eshelf64.c \
	eshelf64_nbsd.c \
	eshlelf64.c \
	eshlelf64_nbsd.c

ALL_64_EMULATIONS = $(ALL_64_EMULATION_SOURCES:.c=.@OBJEXT@)

ALL_EMUL_EXTRA_OFILES = \
	deffilep.@OBJEXT@ \
	pe-dll.@OBJEXT@ \
	pep-dll.@OBJEXT@

CFILES = ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c \
	ldmain.c ldmisc.c ldver.c ldwrite.c lexsup.c \
	mri.c ldcref.c pe-dll.c pep-dll.c ldlex-wrapper.c \
	$(PLUGIN_C)

HFILES = ld.h ldctor.h ldemul.h ldexp.h ldfile.h \
	ldlang.h ldlex.h ldmain.h ldmisc.h ldver.h \
	ldwrite.h mri.h deffile.h pe-dll.h pep-dll.h \
	elf-hints-local.h $(PLUGIN_H)

GENERATED_CFILES = ldgram.c ldlex.c deffilep.c
GENERATED_HFILES = ldgram.h ldemul-list.h deffilep.h

# Require an early dependency on the generated headers, as the dependency
# tracking will not cause them to be built beforehand.
BUILT_SOURCES = $(GENERATED_HFILES)

OFILES = ldgram.@OBJEXT@ ldlex-wrapper.@OBJEXT@ lexsup.@OBJEXT@ ldlang.@OBJEXT@ \
	mri.@OBJEXT@ ldctor.@OBJEXT@ ldmain.@OBJEXT@ $(PLUGIN_OBJECT) \
	ldwrite.@OBJEXT@ ldexp.@OBJEXT@  ldemul.@OBJEXT@ ldver.@OBJEXT@ ldmisc.@OBJEXT@ \
	ldfile.@OBJEXT@ ldcref.@OBJEXT@ ${EMULATION_OFILES} ${EMUL_EXTRA_OFILES}

STAGESTUFF = *.@OBJEXT@ ldscripts/* e*.c

# Disable -Werror, if it has been enabled, since old versions of bison/
# yacc will produce working code which contain compile time warnings.
ldgram.@OBJEXT@: ldgram.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f ldgram.c || echo $(srcdir)/`ldgram.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldgram.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f ldgram.c || echo $(srcdir)/`ldgram.c $(NO_WERROR)
endif

ldlex-wrapper.@OBJEXT@: ldlex-wrapper.c ldlex.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $(srcdir)/ldlex-wrapper.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldlex-wrapper.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c $(srcdir)/ldlex-wrapper.c $(NO_WERROR)
endif

deffilep.@OBJEXT@: deffilep.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f deffilep.c || echo $(srcdir)/`deffilep.c $(NO_WERROR)
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='deffilep.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c `test -f deffilep.c || echo $(srcdir)/`deffilep.c $(NO_WERROR)
endif

# At the moment this is just a list of those emulation template files
# that contain internationalised strings.
EMULATION_FILES = emultempl/pe.em emultempl/armcoff.em

POTFILES = $(CFILES) $(HFILES) $(EMULATION_FILES)

po/POTFILES.in: @MAINT@ Makefile
	for f in $(POTFILES); do echo $$f; done | LC_ALL=C sort > tmp \
	  && mv tmp $(srcdir)/po/POTFILES.in

ldmain.@OBJEXT@: ldmain.c config.status
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	  -DDEFAULT_EMULATION='"$(EMUL)"' \
	  -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	  -DTARGET='"@target@"' @TARGET_SYSTEM_ROOT_DEFINE@ \
	  $(srcdir)/ldmain.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldmain.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c -DDEFAULT_EMULATION='"$(EMUL)"' \
	  -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	  -DTARGET='"@target@"' @TARGET_SYSTEM_ROOT_DEFINE@ \
	  $(srcdir)/ldmain.c
endif

ldfile.@OBJEXT@: ldfile.c config.status
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	-DSCRIPTDIR='"$(scriptdir)"' -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	 $(srcdir)/ldfile.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldfile.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c -DSCRIPTDIR='"$(scriptdir)"' -DBINDIR='"$(bindir)"' \
	  -DTOOLBINDIR='"$(tooldir)/bin"' \
	  $(srcdir)/ldfile.c
endif

eelf32_spu.@OBJEXT@: eelf32_spu.c
if am__fastdepCC
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	  -DEMBEDSPU="\"`echo embedspu | sed '$(transform)'`\"" eelf32_spu.c
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='eelf32_spu.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(COMPILE) -c -DEMBEDSPU="\"`echo embedspu | sed '$(transform)'`\"" \
	  eelf32_spu.c
endif

ldemul-list.h: Makefile
	(echo "/* This file is automatically generated.  DO NOT EDIT! */";\
	for f in `echo " " ${EMULATION_OFILES} "" \
	 | sed -e 's/ e/ ld/g' -e 's/ ld/ /g' -e 's/[.]o//g'`; do \
	    echo "extern ld_emulation_xfer_type ld_$${f}_emulation;"; \
	done;\
	echo "";\
	echo "#define EMULATION_LIST \\";\
	for f in `echo " " ${EMULATION_OFILES} "" \
	 | sed -e 's/ e/ ld/g' -e 's/ ld/ /g' -e 's/[.]o//g'`; do \
	    echo "  &ld_$${f}_emulation, \\"; \
	done;\
	echo "  0") >ldemul-tmp.h
	mv ldemul-tmp.h ldemul-list.h

stringify.sed: ${srcdir}/emultempl/$(STRINGIFY)
	  cp ${srcdir}/emultempl/$(STRINGIFY) stringify.sed

# These all start with e so 'make clean' can find them.

GENSCRIPTS = LIB_PATH='${LIB_PATH}' $(SHELL) $(srcdir)/genscripts.sh "${srcdir}" "${libdir}" "${prefix}" "${exec_prefix}" @host@ @target@ @target_alias@ "@EMULATION_LIBPATH@" "@NATIVE_LIB_DIRS@" @use_sysroot@ @enable_initfini_array@
GEN_DEPENDS = $(srcdir)/genscripts.sh stringify.sed
ELF_DEPS = $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/elf-generic.em
ELF_GEN_DEPS = $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/elf-generic.em $(srcdir)/emultempl/genelf.em

@TDIRS@

eaix5ppc.c: $(srcdir)/emulparams/aix5ppc.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aix5ppc "$(tdir_aixppc)"
eaix5rs6.c: $(srcdir)/emulparams/aix5rs6.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aix5rs6 "$(tdir_aixrs6)"
eaixppc.c: $(srcdir)/emulparams/aixppc.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aixppc "$(tdir_aixppc)"
eaixrs6.c: $(srcdir)/emulparams/aixrs6.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aixrs6 "$(tdir_aixrs6)"
ealpha.c: $(srcdir)/emulparams/alpha.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/alpha.sc ${GEN_DEPENDS}
	${GENSCRIPTS} alpha "$(tdir_alpha)"
ealphavms.c: $(srcdir)/emulparams/alphavms.sh \
  $(srcdir)/emultempl/vms.em $(srcdir)/scripttempl/alphavms.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} alphavms "$(tdir_alphavms)"
earcelf.c: $(srcdir)/emulparams/arcelf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} arcelf "$(tdir_arcelf)"
earm_epoc_pe.c: $(srcdir)/emulparams/arm_epoc_pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/epocpe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} arm_epoc_pe "$(tdir_armpe)"
earm_wince_pe.c: $(srcdir)/emulparams/arm_wince_pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} arm_wince_pe "$(tdir_armpe)"
earmaoutb.c:	$(srcdir)/emulparams/armaoutb.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/armaout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armaoutb "$(tdir_armaoutb)"
earmaoutl.c:	$(srcdir)/emulparams/armaoutl.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/armaout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armaoutl "$(tdir_armaoutl)"
earmcoff.c:	$(srcdir)/emulparams/armcoff.sh \
  $(srcdir)/emultempl/armcoff.em $(srcdir)/scripttempl/armcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armcoff "$(tdir_armcoff)"
earmelf.c: $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf "$(tdir_armelf)"
earmelf_fbsd.c: $(srcdir)/emulparams/armelf_fbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_fbsd "$(tdir_armelf_fbsd)"
earmelf_linux.c: $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_linux "$(tdir_armelf_linux)"
earmelf_linux_eabi.c: $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_linux_eabi "$(tdir_armelf_linux_abi)"
earmelf_nacl.c: $(srcdir)/emulparams/armelf_nacl.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_nacl "$(tdir_armelf_nacl)"
earmelf_nbsd.c: $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_nbsd "$(tdir_armelf_nbsd)"
earmelf_nbsd_eabi.c: $(srcdir)/emulparams/armelf_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_nbsd_eabi "$(tdir_armelf_nbsd_eabi)"
earmelf_nbsd_eabihf.c: $(srcdir)/emulparams/armelf_nbsd_eabihf.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_nbsd_eabihf "$(tdir_armelf_nbsd_eabihf)"
earmelf_vxworks.c: $(srcdir)/emulparams/armelf_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/vxworks.em \
  $(srcdir)/emultempl/armelf.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} armelf_vxworks "$(tdir_armelf)"
earmelfb.c: $(srcdir)/emulparams/armelfb.sh $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb "$(tdir_armelfb)"
earmelfb_linux.c: $(srcdir)/emulparams/armelfb_linux.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_linux "$(tdir_armelfb_linux)"
earmelfb_linux_eabi.c: $(srcdir)/emulparams/armelfb_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_linux_eabi "$(tdir_armelfb_linux_abi)"
earmelfb_nacl.c: $(srcdir)/emulparams/armelfb_nacl.sh \
  $(srcdir)/emulparams/armelf_nacl.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_nacl "$(tdir_armelfb_nacl)"
earmelfb_nbsd.c: $(srcdir)/emulparams/armelfb_nbsd.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_nbsd "$(tdir_armelfb_nbsd)"
earmelfb_nbsd_eabi.c: $(srcdir)/emulparams/armelfb_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_nbsd_eabi "$(tdir_armelfb_nbsd_eabi)"
earmelfb_nbsd_eabihf.c: $(srcdir)/emulparams/armelfb_nbsd_eabihf.sh \
  $(srcdir)/emulparams/armelf_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armelfb_nbsd_eabihf "$(tdir_armelfb_nbsd_eabihf)"
earmnbsd.c:	$(srcdir)/emulparams/armnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armnbsd "$(tdir_armnbsd)"
earmnto.c: $(srcdir)/emulparams/armnto.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armnto "$(tdir_armnto)"
earmpe.c: $(srcdir)/emulparams/armpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} armpe "$(tdir_armpe)"
earmsymbian.c: $(srcdir)/emulparams/armsymbian.sh \
  $(srcdir)/emulparams/armelf.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/armelf.em $(srcdir)/scripttempl/armbpabi.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} armsymbian "$(tdir_armelf)"
eavr1.c: $(srcdir)/emulparams/avr1.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr1 "$(tdir_avr2)"
eavr2.c: $(srcdir)/emulparams/avr2.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr2 "$(tdir_avr2)"
eavr25.c: $(srcdir)/emulparams/avr25.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr25 "$(tdir_avr2)"
eavr3.c: $(srcdir)/emulparams/avr3.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr3 "$(tdir_avr2)"
eavr31.c: $(srcdir)/emulparams/avr31.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr31 "$(tdir_avr2)"
eavr35.c: $(srcdir)/emulparams/avr35.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr35 "$(tdir_avr2)"
eavr4.c: $(srcdir)/emulparams/avr4.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr4 "$(tdir_avr2)"
eavr5.c: $(srcdir)/emulparams/avr5.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr5 "$(tdir_avr2)"
eavr51.c: $(srcdir)/emulparams/avr51.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr51 "$(tdir_avr2)"
eavr6.c: $(srcdir)/emulparams/avr6.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avr6 "$(tdir_avr2)"
eavrxmega1.c: $(srcdir)/emulparams/avrxmega1.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega1 "$(tdir_avr2)"
eavrxmega2.c: $(srcdir)/emulparams/avrxmega2.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega2 "$(tdir_avr2)"
eavrxmega3.c: $(srcdir)/emulparams/avrxmega3.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega3 "$(tdir_avr2)"
eavrxmega4.c: $(srcdir)/emulparams/avrxmega4.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega4 "$(tdir_avr2)"
eavrxmega5.c: $(srcdir)/emulparams/avrxmega5.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega5 "$(tdir_avr2)"
eavrxmega6.c: $(srcdir)/emulparams/avrxmega6.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega6 "$(tdir_avr2)"
eavrxmega7.c: $(srcdir)/emulparams/avrxmega7.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} avrxmega7 "$(tdir_avr2)"
ecoff_i860.c: $(srcdir)/emulparams/coff_i860.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i860coff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} coff_i860 "$(tdir_coff_i860)"
ecoff_sparc.c: $(srcdir)/emulparams/coff_sparc.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sparccoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} coff_sparc "$(tdir_coff_sparc)"
ecrisaout.c: $(srcdir)/emulparams/crisaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/crisaout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} crisaout "$(tdir_cris)"
ecriself.c: $(srcdir)/emulparams/criself.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} criself "$(tdir_cris)"
ecrislinux.c: $(srcdir)/emulparams/crislinux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} crislinux "$(tdir_cris)"
ed10velf.c: $(srcdir)/emulparams/d10velf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfd10v.sc ${GEN_DEPENDS}
	${GENSCRIPTS} d10velf "$(tdir_d10v)"
ed30v_e.c: $(srcdir)/emulparams/d30v_e.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}
	${GENSCRIPTS} d30v_e "$(tdir_d30v)"
ed30v_o.c: $(srcdir)/emulparams/d30v_o.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}
	${GENSCRIPTS} d30v_o "$(tdir_d30v)"
ed30velf.c: $(srcdir)/emulparams/d30velf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}
	${GENSCRIPTS} d30velf "$(tdir_d30v)"
edelta68.c: $(srcdir)/emulparams/delta68.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/delta68.sc ${GEN_DEPENDS}
	${GENSCRIPTS} delta68 "$(tdir_delta68)"
eelf32_dlx.c: $(srcdir)/emulparams/elf32_dlx.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/dlx.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_dlx "$(tdir_elf32_dlx)"
eelf32_i860.c: $(srcdir)/emulparams/elf32_i860.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_i860 "$(tdir_elf32_i860)"
eelf32_i960.c: $(srcdir)/emulparams/elf32_i960.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_i960 "$(tdir_elf32_i960)"
eelf32_sparc.c: $(srcdir)/emulparams/elf32_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_sparc "$(tdir_elf32_sparc)"
eelf32_sparc_sol2.c: $(srcdir)/emulparams/elf32_sparc_sol2.sh \
  $(srcdir)/emulparams/elf32_sparc.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_sparc_sol2 "$(tdir_elf32_sparc_sol2)"
eelf32_sparc_vxworks.c: $(srcdir)/emulparams/elf32_sparc_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emulparams/elf32_sparc.sh \
  $(srcdir)/emultempl/vxworks.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_sparc_vxworks "$(tdir_elf32_sparc_vxworks)"
eelf32_spu.c: $(srcdir)/emulparams/elf32_spu.sh $(srcdir)/emultempl/spuelf.em \
  $(srcdir)/emultempl/spu_ovl.@OBJEXT@_c $(srcdir)/emultempl/spu_icache.@OBJEXT@_c \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_spu "$(tdir_elf32_spu)"
$(srcdir)/emultempl/spu_ovl.@OBJEXT@_c: @MAINT@ $(srcdir)/emultempl/spu_ovl.S
	if ../gas/as-new --version \
		| grep 'target.*spu' >/dev/null 2>/dev/null; then \
	  cpp -DOVLY_IRQ_SAVE $(srcdir)/emultempl/spu_ovl.S spu_ovl.s; \
	  ../gas/as-new -o spu_ovl.@OBJEXT@ spu_ovl.s; \
	  ../binutils/bin2c <spu_ovl.@OBJEXT@ >$@; \
	fi
$(srcdir)/emultempl/spu_icache.@OBJEXT@_c: @MAINT@ $(srcdir)/emultempl/spu_icache.S
	if ../gas/as-new --version \
		| grep 'target.*spu' >/dev/null 2>/dev/null; then \
	  cpp -DOVLY_IRQ_SAVE $(srcdir)/emultempl/spu_icache.S spu_icache.s; \
	  ../gas/as-new -o spu_icache.@OBJEXT@ spu_icache.s; \
	  ../binutils/bin2c <spu_icache.@OBJEXT@ >$@; \
	fi
eelf32_tic6x_be.c: $(srcdir)/emulparams/elf32_tic6x_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_be "$(tdir_elf32_tic6x_be)"
eelf32_tic6x_elf_be.c: $(srcdir)/emulparams/elf32_tic6x_elf_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_elf_be "$(tdir_elf32_tic6x_elf_be)"
eelf32_tic6x_elf_le.c: $(srcdir)/emulparams/elf32_tic6x_elf_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_elf_le "$(tdir_elf32_tic6x_elf_le)"
eelf32_tic6x_le.c: $(srcdir)/emulparams/elf32_tic6x_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_le "$(tdir_elf32_tic6x_le)"
eelf32_tic6x_linux_be.c: $(srcdir)/emulparams/elf32_tic6x_linux_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_linux_be "$(tdir_elf32_tic6x_linux_be)"
eelf32_tic6x_linux_le.c: $(srcdir)/emulparams/elf32_tic6x_linux_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_tic6x_linux_le "$(tdir_elf32_tic6x_linux_le)"
eelf32am33lin.c: $(srcdir)/emulparams/elf32am33lin.sh \
  $(srcdir)/emulparams/elf32am33lin.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32am33lin "$(tdir_elf32am33lin)"
eelf32b4300.c: $(srcdir)/emulparams/elf32b4300.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32b4300 "$(tdir_elf32b4300)"
eelf32bfin.c: $(srcdir)/emulparams/bfin.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/bfin.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32bfin "$(tdir_elf32bfin)" bfin
eelf32bfinfd.c: $(srcdir)/emulparams/elf32bfinfd.sh \
  $(srcdir)/emulparams/bfin.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/bfin.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32bfinfd "$(tdir_elf32bfinfd)" elf32bfinfd
eelf32bmip.c: $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32bmip "$(tdir_elf32bmip)"
eelf32bmipn32.c: $(srcdir)/emulparams/elf32bmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/irix.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32bmipn32 "$(tdir_elf32bmipn32)"
eelf32bsmip.c: $(srcdir)/emulparams/elf32bsmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) $(srcdir)/emultempl/irix.em \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32bsmip "$(tdir_elf32bsmip)"
eelf32btsmip.c: $(srcdir)/emulparams/elf32btsmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32btsmip "$(tdir_elf32btsmip)"
eelf32btsmip_fbsd.c: $(srcdir)/emulparams/elf32btsmip_fbsd.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32btsmip_fbsd "$(tdir_elf32btsmip_fbsd)"
eelf32btsmipn32.c: $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32btsmipn32 "$(tdir_elf32btsmipn32)"
eelf32btsmipn32_fbsd.c: $(srcdir)/emulparams/elf32btsmipn32_fbsd.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32btsmipn32_fbsd "$(tdir_elf32btsmipn32_fbsd)"
eelf32cr16.c: $(srcdir)/emulparams/elf32cr16.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/cr16elf.em \
  $(srcdir)/scripttempl/elf32cr16.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32cr16 "$(tdir_elf32crx)"
eelf32cr16c.c: $(srcdir)/emulparams/elf32cr16c.sh \
  $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf32cr16c.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32cr16c "$(tdir_elf32cr16c)"
eelf32crx.c: $(srcdir)/emulparams/elf32crx.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/crxelf.em \
  $(srcdir)/scripttempl/elf32crx.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32crx "$(tdir_elf32crx)"
eelf32ebmip.c: $(srcdir)/emulparams/elf32ebmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ebmip "$(tdir_elf32ebmip)"
eelf32ebmipvxworks.c: $(srcdir)/emulparams/elf32ebmipvxworks.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/emultempl/vxworks.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ebmipvxworks "$(tdir_elf32ebmipvxworks)"
eelf32elmip.c: $(srcdir)/emulparams/elf32elmip.sh \
  $(srcdir)/emulparams/elf32lmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32elmip "$(tdir_elf32elmip)"
eelf32elmipvxworks.c: $(srcdir)/emulparams/elf32elmipvxworks.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/emultempl/vxworks.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32elmipvxworks "$(tdir_elf32elmipvxworks)"
eelf32epiphany.c: $(srcdir)/emulparams/elf32epiphany.sh \
  $(ELF_DEPS) ${GEN_DEPENDS}
	${GENSCRIPTS} elf32epiphany "$(tdir_epiphany)"
eelf32fr30.c: $(srcdir)/emulparams/elf32fr30.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32fr30 "$(tdir_fr30)"
eelf32frv.c: $(srcdir)/emulparams/elf32frv.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32frv "$(tdir_frv)"
eelf32frvfd.c: $(srcdir)/emulparams/elf32frvfd.sh \
  $(srcdir)/emulparams/elf32frv.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32frvfd "$(tdir_frv)"
eelf32i370.c: $(srcdir)/emulparams/elf32i370.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfi370.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32i370 "$(tdir_elf32i370)"
eelf32ip2k.c: $(srcdir)/emulparams/elf32ip2k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ip2k "$(tdir_ip2k)"
eelf32iq10.c: $(srcdir)/emulparams/elf32iq10.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/iq2000.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32iq10 "$(tdir_iq10)"
eelf32iq2000.c: $(srcdir)/emulparams/elf32iq2000.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/iq2000.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32iq2000 "$(tdir_iq2000)"
eelf32l4300.c: $(srcdir)/emulparams/elf32l4300.sh \
  $(srcdir)/emulparams/elf32b4300.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32l4300 "$(tdir_elf32l4300)"
eelf32lm32.c: $(srcdir)/emulparams/elf32lm32.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lm32 "$(tdir_elf32lm32)"
eelf32lm32fd.c: $(srcdir)/emulparams/elf32lm32fd.sh \
  $(srcdir)/emulparams/elf32lm32.sh $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lm32fd "$(tdir_elf32lm32fd)"
eelf32lmip.c: $(srcdir)/emulparams/elf32lmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lmip "$(tdir_elf32lmip)"
eelf32lppc.c: $(srcdir)/emulparams/elf32lppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emultempl/ppc32elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lppc "$(tdir_elf32lppc)"
eelf32lppcnto.c: $(srcdir)/emulparams/elf32lppcnto.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lppcnto "$(tdir_elf32lppcnto)"
eelf32lppcsim.c: $(srcdir)/emulparams/elf32lppcsim.sh \
  $(srcdir)/emulparams/elf32lppc.sh $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh $(srcdir)/emultempl/ppc32elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lppcsim "$(tdir_elf32lppcsim)"
eelf32lriscv.c: $(srcdir)/emulparams/elf32lriscv.sh \
  $(srcdir)/emulparams/elf32lriscv-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/riscvelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lriscv "$(tdir_elf32lriscv)"
eelf32lsmip.c: $(srcdir)/emulparams/elf32lsmip.sh \
  $(srcdir)/emulparams/elf32lmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32lsmip "$(tdir_elf32lsmip)"
eelf32ltsmip.c: $(srcdir)/emulparams/elf32ltsmip.sh \
  $(srcdir)/emulparams/elf32btsmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ltsmip "$(tdir_elf32ltsmip)"
eelf32ltsmip_fbsd.c: $(srcdir)/emulparams/elf32ltsmip_fbsd.sh \
  $(srcdir)/emulparams/elf32btsmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ltsmip_fbsd "$(tdir_elf32ltsmip_fbsd)"
eelf32ltsmipn32.c: $(srcdir)/emulparams/elf32ltsmipn32.sh \
  $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ltsmipn32 "$(tdir_elf32ltsmipn32)"
eelf32ltsmipn32_fbsd.c: $(srcdir)/emulparams/elf32ltsmipn32_fbsd.sh \
  $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ltsmipn32_fbsd "$(tdir_elf32ltsmipn32_fbsd)"
eelf32m32c.c: $(srcdir)/emulparams/elf32m32c.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32m32c "$(tdir_m32c)"
eelf32mb_linux.c: $(srcdir)/emulparams/elf32mb_linux.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32mb_linux "$(tdir_microblaze)"
eelf32mcore.c: $(srcdir)/emulparams/elf32mcore.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32mcore "$(tdir_mcore)"
eelf32mep.c: $(srcdir)/emulparams/elf32mep.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/mep.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32mep "$(tdir_mep)"
eelf32microblaze.c: $(srcdir)/emulparams/elf32microblaze.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elfmicroblaze.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32microblaze "$(tdir_microblaze)"
eelf32mipswindiss.c: $(srcdir)/emulparams/elf32mipswindiss.sh $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32mipswindiss "$(tdir_elf32mipswindiss)"
eelf32moxie.c: $(srcdir)/emulparams/elf32moxie.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32moxie "$(tdir_moxie)"
eelf32mt.c: $(srcdir)/emulparams/elf32mt.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32mt "$(tdir_mt)"
eelf32or1k.c: $(srcdir)/emulparams/elf32or1k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32or1k "$(tdir_or1k)"
eelf32or1k_linux.c: $(srcdir)/emulparams/elf32or1k_linux.sh \
  $(srcdir)/emulparams/elf32or1k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32or1k_linux "$(tdir_or1k_linux)"
eelf32or1k_nbsd.c: $(srcdir)/emulparams/elf32or1k_nbsd.sh \
  $(srcdir)/emulparams/elf32or1k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32or1k_nbsd "$(tdir_or1k_nbsd)"
eelf32ppc.c: $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh $(srcdir)/emultempl/ppc32elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppc "$(tdir_elf32ppc)"
eelf32ppc_fbsd.c: $(srcdir)/emulparams/elf32ppc_fbsd.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppc_fbsd "$(tdir_elf32ppc_fbsd)"
eelf32ppc_nbsd.c: $(srcdir)/emulparams/elf32ppc_nbsd.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppc_nbsd "$(tdir_elf32ppc_nbsd)"
eelf32ppclinux.c: $(srcdir)/emulparams/elf32ppclinux.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppclinux "$(tdir_elf32ppclinux)"
eelf32ppcnto.c: $(srcdir)/emulparams/elf32ppcnto.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppcnto "$(tdir_elf32ppcnto)"
eelf32ppcsim.c: $(srcdir)/emulparams/elf32ppcsim.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppcsim "$(tdir_elf32ppcsim)"
eelf32ppcvxworks.c: $(srcdir)/emulparams/elf32ppcvxworks.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh $(srcdir)/emulparams/vxworks.sh \
  $(srcdir)/emultempl/vxworks.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppcvxworks "$(tdir_elf32ppcvxworks)"
eelf32ppcwindiss.c: $(srcdir)/emulparams/elf32ppcwindiss.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32ppcwindiss "$(tdir_elf32ppcwindiss)"
eelf32rl78.c: $(srcdir)/emulparams/elf32rl78.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32rl78 "$(tdir_elf32rl78)"
eelf32rx.c: $(srcdir)/emulparams/elf32rx.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32rx "$(tdir_elf32rx)"
eelf32tilegx.c: $(srcdir)/emulparams/elf32tilegx.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32tilegx "$(tdir_tilegx)"
eelf32tilegx_be.c: $(srcdir)/emulparams/elf32tilegx_be.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32tilegx_be "$(tdir_tilegx_be)"
eelf32tilepro.c: $(srcdir)/emulparams/elf32tilepro.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32tilepro "$(tdir_tilepro)"
eelf32vax.c: $(srcdir)/emulparams/elf32vax.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32vax "$(tdir_elf32vax)"
eelf32xc16x.c: $(srcdir)/emulparams/elf32xc16x.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32xc16x "$(tdir_xc16x)"
eelf32xc16xl.c: $(srcdir)/emulparams/elf32xc16xl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32xc16xl "$(tdir_xc16xl)"
eelf32xc16xs.c: $(srcdir)/emulparams/elf32xc16xs.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32xc16xs "$(tdir_xc16xs)"
eelf32xstormy16.c: $(srcdir)/emulparams/elf32xstormy16.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/xstormy16.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32xstormy16 "$(tdir_xstormy16)"
eelf32xtensa.c: $(srcdir)/emulparams/elf32xtensa.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/xtensaelf.em $(INCDIR)/xtensa-config.h \
  $(BFDDIR)/elf-bfd.h $(BFDDIR)/libbfd.h $(INCDIR)/elf/xtensa.h \
  $(srcdir)/scripttempl/elfxtensa.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32xtensa "$(tdir_elf32xtensa)"
eelf_i386.c: $(srcdir)/emulparams/elf_i386.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386 "$(tdir_elf_i386)"
eelf_i386_be.c: $(srcdir)/emulparams/elf_i386_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_be "$(tdir_elf_i386_be)"
eelf_i386_chaos.c: $(srcdir)/emulparams/elf_i386_chaos.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf_chaos.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_chaos "$(tdir_elf_i386_chaos)"
eelf_i386_fbsd.c: $(srcdir)/emulparams/elf_i386_fbsd.sh \
  $(srcdir)/emulparams/elf_i386.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_fbsd "$(tdir_elf_i386_fbsd)"
eelf_i386_ldso.c: $(srcdir)/emulparams/elf_i386_ldso.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_ldso "$(tdir_elf_i386_ldso)"
eelf_i386_nacl.c: $(srcdir)/emulparams/elf_i386_nacl.sh \
  $(srcdir)/emulparams/elf_i386.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_nacl "$(tdir_elf_i386_nacl)"
eelf_i386_sol2.c: $(srcdir)/emulparams/elf_i386_sol2.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_sol2 "$(tdir_elf_i386_sol2)"
eelf_i386_vxworks.c: $(srcdir)/emulparams/elf_i386_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emultempl/vxworks.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_i386_vxworks "$(tdir_elf_i386_vxworks)"
eelf_s390.c: $(srcdir)/emulparams/elf_s390.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_s390 "$(tdir_elf_s390)"
egld960.c: $(srcdir)/emulparams/gld960.sh \
  $(srcdir)/emultempl/gld960.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}
	${GENSCRIPTS} gld960 "$(tdir_gld960)"
egld960coff.c: $(srcdir)/emulparams/gld960coff.sh \
  $(srcdir)/emultempl/gld960c.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}
	${GENSCRIPTS} gld960coff "$(tdir_gld960coff)"
eh8300.c: $(srcdir)/emulparams/h8300.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300 "$(tdir_h8300)"
eh8300elf.c: $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300elf "$(tdir_h8300elf)"
eh8300h.c: $(srcdir)/emulparams/h8300h.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300h.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300h "$(tdir_h8300h)"
eh8300helf.c: $(srcdir)/emulparams/h8300helf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300helf "$(tdir_h8300helf)"
eh8300hn.c: $(srcdir)/emulparams/h8300hn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300hn.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300hn "$(tdir_h8300hn)"
eh8300hnelf.c: $(srcdir)/emulparams/h8300hnelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300hnelf "$(tdir_h8300hnelf)"
eh8300s.c: $(srcdir)/emulparams/h8300s.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300s.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300s "$(tdir_h8300s)"
eh8300self.c: $(srcdir)/emulparams/h8300self.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300self "$(tdir_h8300self)"
eh8300sn.c: $(srcdir)/emulparams/h8300sn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sn.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300sn "$(tdir_h8300sn)"
eh8300snelf.c: $(srcdir)/emulparams/h8300snelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300snelf "$(tdir_h8300snelf)"
eh8300sx.c: $(srcdir)/emulparams/h8300sx.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sx.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300sx "$(tdir_h8300sx)"
eh8300sxelf.c: $(srcdir)/emulparams/h8300sxelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300sxelf "$(tdir_h8300sxelf)"
eh8300sxn.c: $(srcdir)/emulparams/h8300sxn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sxn.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300sxn "$(tdir_h8300sxn)"
eh8300sxnelf.c: $(srcdir)/emulparams/h8300sxnelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8300sxnelf "$(tdir_h8300sxnelf)"
eh8500.c: $(srcdir)/emulparams/h8500.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8500 "$(tdir_h8500)"
eh8500b.c: $(srcdir)/emulparams/h8500b.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500b.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8500b "$(tdir_h8500b)"
eh8500c.c: $(srcdir)/emulparams/h8500c.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500c.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8500c "$(tdir_h8500c)"
eh8500m.c: $(srcdir)/emulparams/h8500m.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500m.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8500m "$(tdir_h8500m)"
eh8500s.c: $(srcdir)/emulparams/h8500s.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500s.sc ${GEN_DEPENDS}
	${GENSCRIPTS} h8500s "$(tdir_h8500s)"
ehp300bsd.c: $(srcdir)/emulparams/hp300bsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hp300bsd "$(tdir_hp300bsd)"
ehp3hpux.c: $(srcdir)/emulparams/hp3hpux.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hp3hpux "$(tdir_hp3hpux)"
ehppaelf.c: $(srcdir)/emulparams/hppaelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/hppaelf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hppaelf "$(tdir_hppaelf)"
ehppalinux.c: $(srcdir)/emulparams/hppalinux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hppalinux "$(tdir_hppalinux)"
ehppanbsd.c: $(srcdir)/emulparams/hppanbsd.sh \
  $(srcdir)/emulparams/hppaelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hppanbsd "$(tdir_hppanbsd)"
ehppaobsd.c: $(srcdir)/emulparams/hppaobsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hppaobsd "$(tdir_hppaobsd)"
ei386aout.c: $(srcdir)/emulparams/i386aout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386aout "$(tdir_i386aout)"
ei386beos.c: $(srcdir)/emulparams/i386beos.sh \
  $(srcdir)/emultempl/beos.em $(srcdir)/scripttempl/i386beos.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386beos "$(tdir_i386beos)"
ei386bsd.c: $(srcdir)/emulparams/i386bsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386bsd "$(tdir_i386bsd)"
ei386coff.c: $(srcdir)/emulparams/i386coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386coff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386coff "$(tdir_i386coff)"
ei386go32.c: $(srcdir)/emulparams/i386go32.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386go32.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386go32 "$(tdir_i386go32)"
ei386linux.c: $(srcdir)/emulparams/i386linux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386linux "$(tdir_i386linux)"
ei386lynx.c: $(srcdir)/emulparams/i386lynx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386lynx "$(tdir_i386lynx)"
ei386mach.c: $(srcdir)/emulparams/i386mach.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386mach "$(tdir_i386mach)"
ei386moss.c: $(srcdir)/emulparams/i386moss.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386moss "$(tdir_i386moss)"
ei386msdos.c: $(srcdir)/emulparams/i386msdos.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386msdos.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386msdos "$(tdir_i386msdos)"
ei386nbsd.c:	$(srcdir)/emulparams/i386nbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386nbsd "$(tdir_i386nbsd)"
ei386nto.c:	$(srcdir)/emulparams/i386nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386nto "$(tdir_i386nto)"
ei386nw.c:	$(srcdir)/emulparams/i386nw.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/nw.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386nw "$(tdir_i386nw)"
ei386pe.c: $(srcdir)/emulparams/i386pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386pe "$(tdir_i386pe)"
ei386pe_posix.c: $(srcdir)/emulparams/i386pe_posix.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386pe_posix "$(tdir_i386pe_posix)"
ei386pep.c: $(srcdir)/emulparams/i386pep.sh \
  $(srcdir)/emultempl/pep.em $(srcdir)/scripttempl/pep.sc ${GEN_DEPENDS}
	${GENSCRIPTS} i386pep "$(tdir_i386pe)"
elnk960.c: $(srcdir)/emulparams/lnk960.sh \
  $(srcdir)/emultempl/lnk960.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}
	${GENSCRIPTS} lnk960 "$(tdir_lnk960)"
em32relf.c: $(srcdir)/emulparams/m32relf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m32relf "$(tdir_m32r)"
em32relf_linux.c: $(srcdir)/emulparams/m32relf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m32relf_linux "$(tdir_m32relf_linux)"
em32rlelf.c: $(srcdir)/emulparams/m32rlelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m32rlelf "$(tdir_m32rlelf)"
em32rlelf_linux.c: $(srcdir)/emulparams/m32rlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m32rlelf_linux "$(tdir_m32rlelf_linux)"
em68hc11elf.c: $(srcdir)/emulparams/m68hc11elf.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc11.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68hc11elf "$(tdir_m68hc11)"
em68hc11elfb.c: $(srcdir)/emulparams/m68hc11elfb.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc11.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68hc11elfb "$(tdir_m68hc11b)"
em68hc12elf.c: $(srcdir)/emulparams/m68hc12elf.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc12.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68hc12elf "$(tdir_m68hc12)"
em68hc12elfb.c: $(srcdir)/emulparams/m68hc12elfb.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc12.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68hc12elfb "$(tdir_m68hc12b)"
em68k4knbsd.c:	$(srcdir)/emulparams/m68k4knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68k4knbsd "$(tdir_m68k4knbsd)"
em68kaout.c: $(srcdir)/emulparams/m68kaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kaout "$(tdir_m68kaout)"
em68kaux.c: $(srcdir)/emulparams/m68kaux.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/m68kaux.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kaux "$(tdir_m68kaux)"
em68kcoff.c: $(srcdir)/emulparams/m68kcoff.sh \
  $(srcdir)/emultempl/m68kcoff.em $(srcdir)/scripttempl/m68kcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kcoff "$(tdir_m68kcoff)"
em68kelf.c: $(srcdir)/emulparams/m68kelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/m68kelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kelf "$(tdir_m68kelf)"
em68kelfnbsd.c: $(srcdir)/emulparams/m68kelfnbsd.sh \
  $(srcdir)/emulparams/m68kelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/m68kelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kelfnbsd "$(tdir_m68kelfnbsd)"
em68klinux.c: $(srcdir)/emulparams/m68klinux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68klinux "$(tdir_m68klinux)"
em68knbsd.c:	$(srcdir)/emulparams/m68knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68knbsd "$(tdir_m68knbsd)"
em68kpsos.c:	$(srcdir)/emulparams/m68kpsos.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/psos.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m68kpsos "$(tdir_m68kpsos)"
em88kbcs.c: $(srcdir)/emulparams/m88kbcs.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/m88kbcs.sc ${GEN_DEPENDS}
	${GENSCRIPTS} m88kbcs "$(tdir_m88kbcs)"
emcorepe.c: $(srcdir)/emulparams/mcorepe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mcorepe "$(tdir_mcorepe)"
emipsbig.c: $(srcdir)/emulparams/mipsbig.sh $(srcdir)/emultempl/generic.em \
  $(srcdir)/scripttempl/mips.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mipsbig "$(tdir_mipsbig)"
emipsbsd.c: $(srcdir)/emulparams/mipsbsd.sh $(srcdir)/emultempl/generic.em \
  $(srcdir)/scripttempl/mipsbsd.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mipsbsd "$(tdir_mipsbsd)"
emipsidt.c: $(srcdir)/emulparams/mipsidt.sh $(srcdir)/emultempl/generic.em \
  $(srcdir)/emultempl/mipsecoff.em $(srcdir)/scripttempl/mips.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} mipsidt "$(tdir_mipsidt)"
emipsidtl.c: $(srcdir)/emulparams/mipsidtl.sh $(srcdir)/emultempl/generic.em \
  $(srcdir)/emultempl/mipsecoff.em $(srcdir)/scripttempl/mips.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} mipsidtl "$(tdir_mipsidtl)"
emipslit.c: $(srcdir)/emulparams/mipslit.sh $(srcdir)/emultempl/generic.em \
  $(srcdir)/scripttempl/mips.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mipslit "$(tdir_mipslit)"
emipslnews.c: $(srcdir)/emulparams/mipslnews.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/mips.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mipslnews "$(tdir_mipslnews)"
emipspe.c: $(srcdir)/emulparams/mipspe.sh $(srcdir)/emultempl/pe.em \
  $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mipspe "$(tdir_mipspe)"
emn10200.c: $(srcdir)/emulparams/mn10200.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mn10200 "$(tdir_mn10200)"
emn10300.c: $(srcdir)/emulparams/mn10300.sh \
  $(srcdir)/emulparams/mn10200.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mn10300 "$(tdir_mn10300)"
emsp430x110.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x110 "$(tdir_msp430x110)" msp430all
emsp430x1101.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1101 "$(tdir_msp430x1101)" msp430all
emsp430x1111.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1111 "$(tdir_msp430x1111)" msp430all
emsp430x112.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x112 "$(tdir_msp430x112)" msp430all
emsp430x1121.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1121 "$(tdir_msp430x1121)" msp430all
emsp430x1122.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1122 "$(tdir_msp430x1122)" msp430all
emsp430x1132.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1132 "$(tdir_msp430x1132)" msp430all
emsp430x122.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x122 "$(tdir_msp430x122)" msp430all
emsp430x1222.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1222 "$(tdir_msp430x1222)" msp430all
emsp430x123.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x123 "$(tdir_msp430x123)" msp430all
emsp430x1232.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1232 "$(tdir_msp430x1232)" msp430all
emsp430x133.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x133 "$(tdir_msp430x133)" msp430all
emsp430x1331.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1331 "$(tdir_msp430x1331)" msp430all
emsp430x135.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x135 "$(tdir_msp430x135)" msp430all
emsp430x1351.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1351 "$(tdir_msp430x1351)" msp430all
emsp430x147.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x147 "$(tdir_msp430x147)" msp430all
emsp430x148.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x148 "$(tdir_msp430x148)" msp430all
emsp430x149.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x149 "$(tdir_msp430x149)" msp430all
emsp430x155.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x155 "$(tdir_msp430x155)" msp430all
emsp430x156.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x156 "$(tdir_msp430x156)" msp430all
emsp430x157.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x157 "$(tdir_msp430x157)" msp430all
emsp430x1610.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1610 "$(tdir_msp430x1610)" msp430all
emsp430x1611.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1611 "$(tdir_msp430x1611)" msp430all
emsp430x1612.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x1612 "$(tdir_msp430x1612)" msp430all
emsp430x167.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x167 "$(tdir_msp430x167)" msp430all
emsp430x168.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x168 "$(tdir_msp430x168)" msp430all
emsp430x169.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x169 "$(tdir_msp430x169)" msp430all
emsp430x2101.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x2101 "$(tdir_msp430x2101)" msp430all
emsp430x2111.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x2111 "$(tdir_msp430x2111)" msp430all
emsp430x2121.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x2121 "$(tdir_msp430x2121)" msp430all
emsp430x2131.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x2131 "$(tdir_msp430x2131)" msp430all
emsp430x311.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x311 "$(tdir_msp430x311)" msp430all
emsp430x312.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x312 "$(tdir_msp430x312)" msp430all
emsp430x313.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x313 "$(tdir_msp430x313)" msp430all
emsp430x314.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x314 "$(tdir_msp430x314)" msp430all
emsp430x315.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x315 "$(tdir_msp430x315)" msp430all
emsp430x323.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x323 "$(tdir_msp430x323)" msp430all
emsp430x325.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x325 "$(tdir_msp430x325)" msp430all
emsp430x336.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x336 "$(tdir_msp430x336)" msp430all
emsp430x337.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430_3.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x337 "$(tdir_msp430x337)" msp430all
emsp430x412.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x412 "$(tdir_msp430x412)" msp430all
emsp430x413.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x413 "$(tdir_msp430x413)" msp430all
emsp430x415.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x415 "$(tdir_msp430x415)" msp430all
emsp430x417.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x417 "$(tdir_msp430x417)" msp430all
emsp430x435.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x435 "$(tdir_msp430x435)" msp430all
emsp430x436.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x436 "$(tdir_msp430x436)" msp430all
emsp430x437.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x437 "$(tdir_msp430x437)" msp430all
emsp430x447.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x447 "$(tdir_msp430x447)" msp430all
emsp430x448.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x448 "$(tdir_msp430x448)" msp430all
emsp430x449.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430x449 "$(tdir_msp430x449)" msp430all
emsp430xE423.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xE423 "$(tdir_msp430xE423)" msp430all
emsp430xE425.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xE425 "$(tdir_msp430xE425)" msp430all
emsp430xE427.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xE427 "$(tdir_msp430xE427)" msp430all
emsp430xG437.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xG437 "$(tdir_msp430xG437)" msp430all
emsp430xG438.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xG438 "$(tdir_msp430xG438)" msp430all
emsp430xG439.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xG439 "$(tdir_msp430xG439)" msp430all
emsp430xW423.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xW423 "$(tdir_msp430xW423)" msp430all
emsp430xW425.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xW425 "$(tdir_msp430xW425)" msp430all
emsp430xW427.c: $(srcdir)/emulparams/msp430all.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc \
  ${GEN_DEPENDS}
	${GENSCRIPTS} msp430xW427 "$(tdir_msp430xW427)" msp430all
enews.c: $(srcdir)/emulparams/news.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} news "$(tdir_news)"
ens32knbsd.c:	$(srcdir)/emulparams/ns32knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/netbsd.em \
  $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} ns32knbsd "$(tdir_ns32knbsd)"
eaarch64elf.c: $(srcdir)/emulparams/aarch64elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64elf "$(tdir_aarch64elf)"
eaarch64elfb.c: $(srcdir)/emulparams/aarch64elfb.sh $(srcdir)/emulparams/aarch64elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64elfb "$(tdir_aarch64elfb)"
eaarch64linux.c: $(srcdir)/emulparams/aarch64linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64linux "$(tdir_aarch64linux)"
eaarch64linuxb.c: $(srcdir)/emulparams/aarch64linuxb.sh $(srcdir)/emulparams/aarch64linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64linuxb "$(tdir_aarch64linuxb)"
eaarch64nbsd.c: $(srcdir)/emulparams/aarch64nbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64nbsd "$(tdir_aarch64nbsd)"
eaarch64nbsdb.c: $(srcdir)/emulparams/aarch64nbsdb.sh $(srcdir)/emulparams/aarch64nbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} aarch64nbsdb "$(tdir_aarch64nbsdb)"
eor32.c: $(srcdir)/emulparams/or32.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/or32.sc ${GEN_DEPENDS}
	${GENSCRIPTS} or32 "$(tdir_or32)"
eor32elf.c: $(srcdir)/emulparams/or32elf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} or32elf "$(tdir_or32elf)"
epc532macha.c: $(srcdir)/emulparams/pc532macha.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} pc532macha "$(tdir_pc532macha)"
epdp11.c: $(srcdir)/emulparams/pdp11.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} pdp11 "$(tdir_pdp11)"
epjelf.c: $(srcdir)/emulparams/pjelf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} pjelf "$(tdir_pjelf)"
epjlelf.c: $(srcdir)/emulparams/pjlelf.sh $(srcdir)/emulparams/pjelf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} pjlelf "$(tdir_pjlelf)"
eppclynx.c: $(srcdir)/emulparams/ppclynx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} ppclynx "$(tdir_ppclynx)"
eppcmacos.c:	$(srcdir)/emulparams/ppcmacos.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}
	${GENSCRIPTS} ppcmacos "$(tdir_ppcmacos)"
eppcnw.c:	$(srcdir)/emulparams/ppcnw.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/nw.sc ${GEN_DEPENDS}
	${GENSCRIPTS} ppcnw "$(tdir_ppcnw)"
eppcpe.c: $(srcdir)/emulparams/ppcpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/ppcpe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} ppcpe "$(tdir_ppcpe)"
eriscix.c: $(srcdir)/emulparams/riscix.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} riscix "$(tdir_riscix)"
escore3_elf.c: $(srcdir)/emulparams/scoreelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/scoreelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} score3_elf "$(tdir_score3_elf)" scoreelf
escore7_elf.c: $(srcdir)/emulparams/scoreelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/scoreelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} score7_elf "$(tdir_score7_elf)" scoreelf
esh.c: $(srcdir)/emulparams/sh.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sh.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sh "$(tdir_sh)"
eshelf.c: $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf "$(tdir_shelf)"
eshelf32.c: $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf32 "$(tdir_shelf32)"
eshelf32_linux.c: $(srcdir)/emulparams/shelf32_linux.sh \
  $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf32_linux "$(tdir_shelf32_linux)"
eshelf32_nbsd.c: $(srcdir)/emulparams/shelf32_nbsd.sh \
  $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf32_nbsd "$(tdir_shelf32_nbsd)"
eshelf_fd.c: $(srcdir)/emulparams/shelf_fd.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_fd "$(tdir_shelf_fd)"
eshelf_linux.c: $(srcdir)/emulparams/shelf_linux.sh \
  $(srcdir)/emulparams/shlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_linux "$(tdir_shelf_linux)"
eshelf_nbsd.c: $(srcdir)/emulparams/shelf_nbsd.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_nbsd "$(tdir_shelf_nbsd)"
eshelf_nto.c: $(srcdir)/emulparams/shelf_nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_nto "$(tdir_shelf_nto)"
eshelf_uclinux.c: $(srcdir)/emulparams/shelf_uclinux.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_uclinux "$(tdir_shelf_uclinux)"
eshelf_vxworks.c: $(srcdir)/emulparams/shelf_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc \
  $(srcdir)/emultempl/vxworks.em ${GEN_DEPENDS}
	${GENSCRIPTS} shelf_vxworks "$(tdir_shelf_vxworks)"
eshl.c: $(srcdir)/emulparams/shl.sh \
  $(srcdir)/emulparams/sh.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sh.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shl "$(tdir_shl)"
eshlelf.c: $(srcdir)/emulparams/shlelf.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf "$(tdir_shlelf)"
eshlelf32.c: $(srcdir)/emulparams/shlelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h $(srcdir)/emulparams/shelf32.sh \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf32 "$(tdir_shlelf32)"
eshlelf32_linux.c: $(srcdir)/emulparams/shlelf32_linux.sh \
  $(srcdir)/emulparams/shelf32_linux.sh $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf32_linux "$(tdir_shlelf32_linux)"
eshlelf32_nbsd.c: $(srcdir)/emulparams/shlelf32_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf32_nbsd "$(tdir_shlelf32_nbsd)"
eshlelf_fd.c: $(srcdir)/emulparams/shlelf_fd.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf_fd "$(tdir_shlelf_fd)"
eshlelf_linux.c: $(srcdir)/emulparams/shlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf_linux "$(tdir_shlelf_linux)"
eshlelf_nbsd.c: $(srcdir)/emulparams/shlelf_nbsd.sh \
  $(srcdir)/emulparams/shelf_nbsd.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf_nbsd "$(tdir_shlelf_nbsd)"
eshlelf_nto.c: $(srcdir)/emulparams/shlelf_nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf_nto "$(tdir_shlelf_nto)"
eshlelf_vxworks.c: $(srcdir)/emulparams/shlelf_vxworks.sh \
  $(srcdir)/emulparams/shelf_vxworks.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc $(srcdir)/emultempl/vxworks.em \
  ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf_vxworks "$(tdir_shlelf_vxworks)"
eshlsymbian.c: $(srcdir)/emulparams/shlsymbian.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf32sh-symbian.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlsymbian "$(tdir_shlelf)"
eshpe.c: $(srcdir)/emulparams/shpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shpe "$(tdir_shl)"
esparcaout.c: $(srcdir)/emulparams/sparcaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sparcaout "$(tdir_sparcaout)"
esparclinux.c: $(srcdir)/emulparams/sparclinux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sparclinux "$(tdir_sparclinux)"
esparcnbsd.c:	$(srcdir)/emulparams/sparcnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sparcnbsd "$(tdir_sparcnbsd)"
est2000.c: $(srcdir)/emulparams/st2000.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/st2000.sc ${GEN_DEPENDS}
	${GENSCRIPTS} st2000 "$(tdir_st2000)"
esun3.c: $(srcdir)/emulparams/sun3.sh \
  $(srcdir)/emultempl/sunos.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sun3 "$(tdir_sun3)"
esun4.c: $(srcdir)/emulparams/sun4.sh \
  $(srcdir)/emultempl/sunos.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} sun4 "$(tdir_sun4)"
etic30aout.c: $(srcdir)/emulparams/tic30aout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic30aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic30aout "$(tdir_tic30aout)"
etic30coff.c: $(srcdir)/emulparams/tic30coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic30coff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic30coff "$(tdir_tic30coff)"
etic3xcoff.c: $(srcdir)/emulparams/tic3xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic3xcoff "$(tdir_tic4xcoff)"
etic3xcoff_onchip.c: $(srcdir)/emulparams/tic3xcoff_onchip.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic3xcoff_onchip "$(tdir_tic4xcoff)"
etic4xcoff.c: $(srcdir)/emulparams/tic4xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic4xcoff "$(tdir_tic4xcoff)"
etic54xcoff.c: $(srcdir)/emulparams/tic54xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic54xcoff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic54xcoff "$(tdir_tic54xcoff)"
etic80coff.c: $(srcdir)/emulparams/tic80coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic80coff.sc ${GEN_DEPENDS}
	${GENSCRIPTS} tic80coff "$(tdir_tic80coff)"
ev850.c: $(srcdir)/emulparams/v850.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/v850.sc ${GEN_DEPENDS}
	${GENSCRIPTS} v850 "$(tdir_v850)"
evanilla.c: $(srcdir)/emulparams/vanilla.sh \
  $(srcdir)/emultempl/vanilla.em $(srcdir)/scripttempl/vanilla.sc ${GEN_DEPENDS}
	${GENSCRIPTS} vanilla "$(tdir_vanilla)"
evax.c: $(srcdir)/emulparams/vax.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} vax "$(tdir_vax)"
evaxnbsd.c: $(srcdir)/emulparams/vaxnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} vaxnbsd "$(tdir_vaxnbsd)"
evsta.c: $(srcdir)/emulparams/vsta.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}
	${GENSCRIPTS} vsta "$(tdir_vsta)"
ew65.c: $(srcdir)/emulparams/w65.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/w65.sc ${GEN_DEPENDS}
	${GENSCRIPTS} w65 "$(tdir_w65)"
exgateelf.c: $(srcdir)/emulparams/xgateelf.sh \
  $(srcdir)/emultempl/generic.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfxgate.sc ${GEN_DEPENDS}
	${GENSCRIPTS} xgateelf "$(tdir_xgate)"
ez80.c: $(srcdir)/emulparams/z80.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/z80.em \
  $(srcdir)/scripttempl/z80.sc ${GEN_DEPENDS}
	${GENSCRIPTS} z80 "$(tdir_z80)"
ez8001.c: $(srcdir)/emulparams/z8001.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/z8000.sc ${GEN_DEPENDS}
	${GENSCRIPTS} z8001 "$(tdir_z8001)"
ez8002.c: $(srcdir)/emulparams/z8002.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/z8000.sc ${GEN_DEPENDS}
	${GENSCRIPTS} z8002 "$(tdir_z8002)"

eelf32_x86_64.c: $(srcdir)/emulparams/elf32_x86_64.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_x86_64 "$(tdir_elf32_x86_64)"
eelf32_x86_64_nacl.c: $(srcdir)/emulparams/elf32_x86_64_nacl.sh \
  $(srcdir)/emulparams/elf32_x86_64.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf32_x86_64_nacl "$(tdir_elf32_x86_64_nacl)"
eelf64_aix.c: $(srcdir)/emulparams/elf64_aix.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_aix "$(tdir_elf64_aix)"
eelf64_ia64.c: $(srcdir)/emulparams/elf64_ia64.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/ia64elf.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_ia64 "$(tdir_elf64_ia64)"
eelf64_ia64_fbsd.c: $(srcdir)/emulparams/elf64_ia64_fbsd.sh \
  $(srcdir)/emulparams/elf64_ia64.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/ia64elf.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_ia64_fbsd "$(tdir_elf64_ia64_fbsd)"
eelf64_ia64_vms.c: $(srcdir)/emulparams/elf64_ia64_vms.sh \
  $(srcdir)/emultempl/vms.em \
  $(srcdir)/scripttempl/ia64vms.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_ia64_vms "$(tdir_elf64_ia64_vms)"
eelf64_s390.c: $(srcdir)/emulparams/elf64_s390.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_s390 "$(tdir_elf64_s390)"
eelf64_sparc.c: $(srcdir)/emulparams/elf64_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_sparc "$(tdir_elf64_sparc)"
eelf64_sparc_fbsd.c: $(srcdir)/emulparams/elf64_sparc_fbsd.sh \
  $(srcdir)/emulparams/elf64_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_sparc_fbsd "$(tdir_elf64_sparc_fbsd)"
eelf64_sparc_sol2.c: $(srcdir)/emulparams/elf64_sparc_sol2.sh \
  $(srcdir)/emulparams/elf64_sparc.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64_sparc_sol2 "$(tdir_elf64_sparc_sol2)"
eelf64alpha.c: $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64alpha "$(tdir_elf64alpha)"
eelf64alpha_fbsd.c: $(srcdir)/emulparams/elf64alpha_fbsd.sh \
  $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64alpha_fbsd "$(tdir_elf64alpha_fbsd)"
eelf64alpha_nbsd.c: $(srcdir)/emulparams/elf64alpha_nbsd.sh \
  $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64alpha_nbsd "$(tdir_elf64alpha_nbsd)"
eelf64bmip.c: $(srcdir)/emulparams/elf64bmip.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/irix.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64bmip "$(tdir_elf64bmip)"
eelf64btsmip.c: $(srcdir)/emulparams/elf64btsmip.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64btsmip "$(tdir_elf64btsmip)"
eelf64btsmip_fbsd.c: $(srcdir)/emulparams/elf64btsmip_fbsd.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64btsmip_fbsd "$(tdir_elf64btsmip_fbsd)"
eelf64hppa.c: $(srcdir)/emulparams/elf64hppa.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf64hppa.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64hppa "$(tdir_elf64hppa)"
eelf64lppc.c: $(srcdir)/emulparams/elf64lppc.sh \
  $(srcdir)/emulparams/elf64ppc.sh $(srcdir)/emultempl/ppc64elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64lppc "$(tdir_elf64lppc)"
eelf64lriscv.c: $(srcdir)/emulparams/elf64lriscv.sh \
  $(srcdir)/emulparams/elf64lriscv-defs.sh \
  $(srcdir)/emulparams/elf32lriscv-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/riscvelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64lriscv "$(tdir_elf64lriscv)"
eelf64ltsmip.c: $(srcdir)/emulparams/elf64ltsmip.sh \
  $(srcdir)/emulparams/elf64btsmip.sh $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64ltsmip "$(tdir_elf64ltsmip)"
eelf64ltsmip_fbsd.c: $(srcdir)/emulparams/elf64ltsmip_fbsd.sh \
  $(srcdir)/emulparams/elf64btsmip_fbsd.sh $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64ltsmip_fbsd "$(tdir_elf64ltsmip_fbsd)"
eelf64mmix.c: $(srcdir)/emulparams/elf64mmix.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mmix-elfnmmo.em \
  $(srcdir)/emultempl/mmixelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64mmix "$(tdir_elf64mmix)"
eelf64ppc.c: $(srcdir)/emulparams/elf64ppc.sh $(srcdir)/emultempl/ppc64elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64ppc "$(tdir_elf64ppc)"
eelf64ppc_fbsd.c: $(srcdir)/emulparams/elf64ppc_fbsd.sh \
  $(srcdir)/emultempl/ppc64elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64ppc_fbsd "$(tdir_elf64ppc_fbsd)"
eelf64tilegx.c: $(srcdir)/emulparams/elf64tilegx.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64tilegx "$(tdir_tilegx)"
eelf64tilegx_be.c: $(srcdir)/emulparams/elf64tilegx_be.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64tilegx_be "$(tdir_tilegx_be)"
eelf_l1om.c: $(srcdir)/emulparams/elf_l1om.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_l1om "$(tdir_elf_l1om)"
eelf_l1om_fbsd.c: $(srcdir)/emulparams/elf_l1om_fbsd.sh \
  $(srcdir)/emulparams/elf_l1om.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_l1om_fbsd "$(tdir_elf_l1om_fbsd)"
eelf_k1om.c: $(srcdir)/emulparams/elf_k1om.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_k1om "$(tdir_elf_k1om)"
eelf_k1om_fbsd.c: $(srcdir)/emulparams/elf_k1om_fbsd.sh \
  $(srcdir)/emulparams/elf_k1om.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_k1om_fbsd "$(tdir_elf_k1om_fbsd)"
eelf_x86_64.c: $(srcdir)/emulparams/elf_x86_64.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_x86_64 "$(tdir_elf_x86_64)"
eelf_x86_64_fbsd.c: $(srcdir)/emulparams/elf_x86_64_fbsd.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_x86_64_fbsd "$(tdir_elf_x86_64_fbsd)"
eelf_x86_64_nacl.c: $(srcdir)/emulparams/elf_x86_64_nacl.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_x86_64_nacl "$(tdir_elf_x86_64_nacl)"
eelf_x86_64_sol2.c: $(srcdir)/emulparams/elf_x86_64_sol2.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf_x86_64_sol2 "$(tdir_elf_x86_64_sol2)"
ehppa64linux.c: $(srcdir)/emulparams/hppa64linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} hppa64linux "$(tdir_hppa64linux)"
emmo.c: $(srcdir)/emulparams/mmo.sh $(srcdir)/emultempl/mmix-elfnmmo.em \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/elf-generic.em \
  $(srcdir)/emultempl/mmo.em \
  $(srcdir)/scripttempl/mmo.sc ${GEN_DEPENDS}
	${GENSCRIPTS} mmo "$(tdir_mmo)"
eshelf64.c: $(srcdir)/emulparams/shelf64.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf64 "$(tdir_shelf64)"
eshelf64_nbsd.c: $(srcdir)/emulparams/shelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shelf64_nbsd "$(tdir_shelf64_nbsd)"
eshlelf64.c: $(srcdir)/emulparams/shlelf64.sh \
  $(srcdir)/emulparams/shelf64.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf64 "$(tdir_shlelf64)"
eshlelf64_nbsd.c: $(srcdir)/emulparams/shlelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} shlelf64_nbsd "$(tdir_shlelf64_nbsd)"

# We need this for automake to use YLWRAP.
EXTRA_ld_new_SOURCES = deffilep.y ldlex.l
# Allow dependency tracking to work for these files, too.
EXTRA_ld_new_SOURCES += pep-dll.c pe-dll.c

ld_new_SOURCES = ldgram.y ldlex-wrapper.c lexsup.c ldlang.c mri.c ldctor.c ldmain.c \
	ldwrite.c ldexp.c ldemul.c ldver.c ldmisc.c ldfile.c ldcref.c $(PLUGIN_C)
ld_new_DEPENDENCIES = $(EMULATION_OFILES) $(EMUL_EXTRA_OFILES) $(BFDLIB) $(LIBIBERTY) $(LIBINTL_DEP)
ld_new_LDADD = $(EMULATION_OFILES) $(EMUL_EXTRA_OFILES) $(BFDLIB) $(LIBIBERTY) $(LIBINTL)

# Dependency tracking for the generated emulation files.
EXTRA_ld_new_SOURCES += $(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES)

# This is the real libbfd.a created by libtool.
TESTBFDLIB = @TESTBFDLIB@

check-DEJAGNU: site.exp
	srcroot=`cd $(srcdir) && pwd`; export srcroot; \
	r=`pwd`; export r; \
	LC_ALL=C; export LC_ALL; \
	EXPECT=$(EXPECT); export EXPECT; \
	runtest=$(RUNTEST); \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  $$runtest --tool $(DEJATOOL) --srcdir $${srcroot}/testsuite \
		CC="$(CC_FOR_TARGET)" CFLAGS="$(CFLAGS)" \
		CXX="$(CXX_FOR_TARGET)" CXXFLAGS="$(CXXFLAGS)" \
		CC_FOR_HOST="$(CC)" CFLAGS_FOR_HOST="$(CFLAGS)" \
		OFILES="$(OFILES)" BFDLIB="$(TESTBFDLIB)" \
		LIBIBERTY="$(LIBIBERTY) $(LIBINTL)" LIBS="$(LIBS)" \
		DO_COMPARE="`echo '$(do_compare)' | sed -e 's,\\$$,,g'`" \
		$(RUNTESTFLAGS); \
	else echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

# Rules for testing by relinking ld itself.
# A similar test is in the testsuite.  This target is for ease of use
# when porting ld.

ld-partial.@OBJEXT@: ld-new$(EXEEXT)
	./ld-new$(EXEEXT) $(HOSTING_EMU) -o ld-partial.@OBJEXT@ -r $(OFILES)
ld1$(EXEEXT): ld-partial.@OBJEXT@
	./ld-new$(EXEEXT) $(HOSTING_EMU) -o ld1$(EXEEXT) $(HOSTING_CRT0) ld-partial.@OBJEXT@ $(TESTBFDLIB) $(LIBIBERTY) $(HOSTING_LIBS) $(LIBS)

ld1-full$(EXEEXT): ld-new
	./ld-new$(EXEEXT) $(HOSTING_EMU) -o ld1-full$(EXEEXT) $(HOSTING_CRT0) $(OFILES) $(TESTBFDLIB) $(LIBIBERTY) $(HOSTING_LIBS) $(LIBS)

ld2$(EXEEXT): ld1$(EXEEXT)
	./ld1$(EXEEXT) $(HOSTING_EMU) -o ld2$(EXEEXT) $(HOSTING_CRT0) $(OFILES) $(TESTBFDLIB) $(LIBIBERTY) $(HOSTING_LIBS) $(LIBS)

ld3$(EXEEXT): ld2$(EXEEXT)
	./ld2$(EXEEXT) $(HOSTING_EMU) -o ld3$(EXEEXT) $(HOSTING_CRT0) $(OFILES) $(TESTBFDLIB) $(LIBIBERTY) $(HOSTING_LIBS) $(LIBS)

bootstrap: ld3$(EXEEXT)
	cmp ld2$(EXEEXT) ld3$(EXEEXT)

.PHONY: bootstrap

# A test program for C++ constructors and destructors.
# This test is now in the testsuite.
#
#cdtest: cdtest-main.@OBJEXT@ cdtest-bar.@OBJEXT@ cdtest-foo.@OBJEXT@ ld.new
#	./ld.new $(HOSTING_EMU) -o cdtest $(HOSTING_CRT0) \
#	  cdtest-main.@OBJEXT@ cdtest-bar.@OBJEXT@ cdtest-foo.@OBJEXT@ $(HOSTING_LIBS)
#
#cdtest.out: cdtest
#	./cdtest > cdtest.tmp
#	mv cdtest.tmp cdtest.out
#
#cdtest-ur.@OBJEXT@: cdtest-main.@OBJEXT@ cdtest-bar.@OBJEXT@ cdtest-foo.@OBJEXT@ ld.new
#	./ld.new $(HOSTING_EMU) -o cdtest-ur.@OBJEXT@ -Ur cdtest-main.@OBJEXT@ \
#	  cdtest-bar.@OBJEXT@ cdtest-foo.@OBJEXT@
#
#cdtest-ur: cdtest-ur.@OBJEXT@
#	./ld.new $(HOSTING_EMU) -o cdtest-ur $(HOSTING_CRT0) cdtest-ur.@OBJEXT@ \
#	  $(HOSTING_LIBS)
#
#cdtest-ur.out: cdtest-ur
#	./cdtest-ur > cdtest-ur.tmp
#	mv cdtest-ur.tmp cdtest-ur.out
#
#check-cdtest: cdtest.out cdtest-ur.out $(srcdir)/cdtest.exp
#	diff $(srcdir)/cdtest.exp cdtest.out
#	diff $(srcdir)/cdtest.exp cdtest-ur.out
#
#.PHONY: check-cdtest

# END OF CHECK TARGETS

#
# Build a dummy plugin using libtool.
#
if ENABLE_PLUGINS
noinst_LTLIBRARIES = libldtestplug.la
libldtestplug_la_SOURCES = testplug.c
libldtestplug_la_CFLAGS= -g -O2
libldtestplug_la_LDFLAGS = -no-undefined -rpath /nowhere
endif

# DOCUMENTATION TARGETS
# Manual configuration file; not usually attached to normal configuration,
# because almost all configs use "gen" version of manual.
#  Set DOCVER above to change.
configdoc.texi:	${DOCVER}-doc.texi
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_configdoc.texi:
	cp ${srcdir}/${DOCVER}-doc.texi ./configdoc.texi
	chmod u+w ./configdoc.texi

# Build the man page from the texinfo file
# The sed command removes the no-adjust Nroff command so that
# the man output looks standard.
ld.1: $(srcdir)/ld.texinfo configdoc.texi
	@echo "NOT REBUILDING $@"
NetBSD_DISABLED_ld.1:
	touch $@
	-$(TEXI2POD) $(MANCONF) < $(srcdir)/ld.texinfo > ld.pod
	-($(POD2MAN) ld.pod | \
		sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || \
		(rm -f $@.T$$$$ && exit 1)
	rm -f ld.pod

MAINTAINERCLEANFILES = configdoc.texi ld.1

# We want to reconfigure if configure.host or configure.tgt changes.  We
# extract version from bfd/configure.in, so we must depend on that also.
CONFIG_STATUS_DEPENDENCIES = $(srcdir)/configure.host $(srcdir)/configure.tgt \
	$(srcdir)/../bfd/configure.in

MOSTLYCLEANFILES = $(STAGESTUFF) ld1$(EXEEXT) ld2$(EXEEXT) ld3$(EXEEXT) \
	ldemul-list.h crtbegin.@OBJEXT@ crtend.@OBJEXT@ ld.log ld.sum
mostlyclean-local:
	-rm -rf tmpdir
CLEANFILES = dep.sed DEP DEPA DEP1 DEP2 spu_ovl.s spu_ovl.@OBJEXT@ spu_icache.s spu_icache.@OBJEXT@

.PHONY: install-exec-local install-data-local

install-exec-local: ld-new$(EXEEXT) install-binPROGRAMS
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/bin
	n=`echo $(installed_linker) | sed '$(transform)'`; \
	if test "$(bindir)" != "$(tooldir)/bin"; then \
	  rm -f $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	fi; \
	if test "x$(install_as_default)" = "xyes"; then \
	  ld=`echo ld | sed '$(transform)'`; \
	  rm -f $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  if test "$(bindir)" != "$(tooldir)/bin"; then \
	    rm -f $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	    ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT) >/dev/null 2>/dev/null \
	    || $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	  fi; \
	fi

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(scriptdir)/ldscripts
	for f in ldscripts/*; do \
	  $(INSTALL_DATA) $$f $(DESTDIR)$(scriptdir)/$$f ; \
	done

# Stuff that should be included in a distribution.  The diststuff
# target is run by the taz target in ../Makefile.in.
EXTRA_DIST = ldgram.c ldgram.h ldlex.c emultempl/spu_ovl.@OBJEXT@_c \
	     emultempl/spu_icache.@OBJEXT@_c deffilep.c deffilep.h $(man_MANS)
diststuff: info $(EXTRA_DIST)

# Both info (ld.info) and ld.1 depend on configdoc.texi.
# But info isn't a direct target. Make info-recursive to depend on
# ld.1 to support parallel build.
info-recursive: ld.1

DISTCLEANFILES = tdirs site.exp site.bak stringify.sed
distclean-local:
	rm -rf ldscripts

MAINTAINERCLEANFILES += ld.info

# Automake 1.9 will only build info files in the objdir if they are
# mentioned in DISTCLEANFILES.  It doesn't have to be unconditional,
# though, so we use a bogus condition.
if GENINSRC_NEVER
DISTCLEANFILES += ld.info
endif
