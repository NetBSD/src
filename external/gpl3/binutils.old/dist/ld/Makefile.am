## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2012-2016 Free Software Foundation, Inc.
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
#

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

ELF_CLFAGS=-DELF_LIST_OPTIONS=@elf_list_options@ \
	   -DELF_SHLIB_LIST_OPTIONS=@elf_shlib_list_options@ \
	   -DELF_PLT_UNWIND_LIST_OPTIONS=@elf_plt_unwind_list_options@
WARN_CFLAGS = @WARN_CFLAGS@
NO_WERROR = @NO_WERROR@
AM_CFLAGS = $(WARN_CFLAGS) $(ELF_CLFAGS)

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
HOSTING_SCRT0 = @HOSTING_SCRT0@
HOSTING_LIBS = @HOSTING_LIBS@
HOSTING_SLIBS = @HOSTING_SLIBS@
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
  elif [ -f $$r/../gcc/xg++ ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/xg++ -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/xg++ -B$$r/../gcc/; \
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
	earcv2elf.c \
	earcv2elfx.c \
	earcelf.c \
	earcelf_prof.c \
	earclinux.c \
	earclinux_prof.c \
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
	earmelf_phoenix.c \
	earmelf_vxworks.c \
	earmelfb.c \
	earmelfb_fbsd.c \
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
	eavrxmega1.c \
	eavrxmega2.c \
	eavrxmega3.c \
	eavrxmega4.c \
	eavrxmega5.c \
	eavrxmega6.c \
	eavrxmega7.c \
	eavrtiny.c   \
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
	eelf32bfin.c \
	eelf32bfinfd.c \
	eelf32cr16.c \
	eelf32cr16c.c \
	eelf32crx.c \
	eelf32epiphany.c \
	eelf32epiphany_4x4.c \
	eelf32fr30.c \
	eelf32frv.c \
	eelf32frvfd.c \
	eelf32ft32.c \
	eelf32i370.c \
	eelf32ip2k.c \
	eelf32iq10.c \
	eelf32iq2000.c \
	eelf32lm32.c \
	eelf32lm32fd.c \
	eelf32lppc.c \
	eelf32lppclinux.c \
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
	eelf32mbel_linux.c \
	eelf32mcore.c \
	eelf32mep.c \
	eelf32metag.c \
	eelf32microblazeel.c \
	eelf32microblaze.c \
	eelf32moxie.c \
	emoxiebox.c \
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
	eelf32visium.c \
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
	eelf_iamcu.c \
	eelf_s390.c \
	egld960.c \
	egld960coff.c \
	eh8300.c \
	eh8300elf.c \
	eh8300elf_linux.c \
	eh8300h.c \
	eh8300helf.c \
	eh8300helf_linux.c \
	eh8300hn.c \
	eh8300hnelf.c \
	eh8300s.c \
	eh8300self.c \
	eh8300self_linux.c \
	eh8300sn.c \
	eh8300snelf.c \
	eh8300sx.c \
	eh8300sxelf.c \
	eh8300sxelf_linux.c \
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
	emn10200.c \
	emn10300.c \
	emsp430elf.c \
	emsp430X.c \
	ends32elf.c \
	ends32elf16m.c \
	ends32elf_linux.c \
	ends32belf.c \
	ends32belf16m.c \
	ends32belf_linux.c \
	enews.c \
	ens32knbsd.c \
	enios2elf.c \
	enios2linux.c \
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
	ev850_rh850.c \
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
	eaarch64elf32.c \
	eaarch64elfb.c \
	eaarch64elf32b.c \
	eaarch64cloudabi.c \
	eaarch64cloudabib.c \
	eaarch64fbsd.c \
	eaarch64fbsdb.c \
	eaarch64linux.c \
	eaarch64linuxb.c \
	eaarch64nbsd.c \
	eaarch64nbsdb.c \
	eaarch64linux32.c \
	eaarch64linux32b.c \
	eelf32_x86_64.c \
	eelf32_x86_64_nacl.c \
	eelf32b4300.c \
	eelf32bmip.c \
	eelf32bmipn32.c \
	eelf32bsmip.c \
	eelf32btsmip.c \
	eelf32btsmip_fbsd.c \
	eelf32btsmipn32.c \
	eelf32btsmipn32_fbsd.c \
	eelf32ebmip.c \
	eelf32ebmipvxworks.c \
	eelf32elmip.c \
	eelf32elmipvxworks.c \
	eelf32l4300.c \
	eelf32lmip.c \
	eelf32lr5900.c \
	eelf32lr5900n32.c \
	eelf32lsmip.c \
	eelf32ltsmip.c \
	eelf32ltsmip_fbsd.c \
	eelf32ltsmipn32.c \
	eelf32ltsmipn32_fbsd.c \
	eelf32mipswindiss.c \
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
	eelf64rdos.c \
	eelf64tilegx.c \
	eelf64tilegx_be.c \
	eelf_l1om.c \
	eelf_l1om_fbsd.c \
	eelf_k1om.c \
	eelf_k1om_fbsd.c \
	eelf_x86_64.c \
	eelf_x86_64_cloudabi.c \
	eelf_x86_64_fbsd.c \
	eelf_x86_64_nacl.c \
	eelf_x86_64_sol2.c \
	ehppa64linux.c \
	ei386pep.c \
	emmo.c \
	eshelf64.c \
	eshelf64_nbsd.c \
	eshlelf64.c \
	eshlelf64_nbsd.c

ALL_64_EMULATIONS = $(ALL_64_EMULATION_SOURCES:.c=.@OBJEXT@)

ALL_EMUL_EXTRA_OFILES = \
	deffilep.@OBJEXT@ \
	pe-dll.@OBJEXT@

ALL_64_EMUL_EXTRA_OFILES = \
	pep-dll.@OBJEXT@

CFILES = ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c \
	ldmain.c ldmisc.c ldver.c ldwrite.c lexsup.c \
	mri.c ldcref.c pe-dll.c pep-dll.c ldlex-wrapper.c \
	$(PLUGIN_C) ldbuildid.c

HFILES = ld.h ldctor.h ldemul.h ldexp.h ldfile.h \
	ldlang.h ldlex.h ldmain.h ldmisc.h ldver.h \
	ldwrite.h mri.h deffile.h pe-dll.h pep-dll.h \
	elf-hints-local.h $(PLUGIN_H) ldbuildid.h

GENERATED_CFILES = ldgram.c ldlex.c deffilep.c
GENERATED_HFILES = ldgram.h ldemul-list.h deffilep.h

# Require an early dependency on the generated headers, as the dependency
# tracking will not cause them to be built beforehand.
BUILT_SOURCES = $(GENERATED_HFILES)

OFILES = ldgram.@OBJEXT@ ldlex-wrapper.@OBJEXT@ lexsup.@OBJEXT@ ldlang.@OBJEXT@ \
	mri.@OBJEXT@ ldctor.@OBJEXT@ ldmain.@OBJEXT@ $(PLUGIN_OBJECT) \
	ldwrite.@OBJEXT@ ldexp.@OBJEXT@  ldemul.@OBJEXT@ ldver.@OBJEXT@ ldmisc.@OBJEXT@ \
	ldfile.@OBJEXT@ ldcref.@OBJEXT@ ${EMULATION_OFILES} ${EMUL_EXTRA_OFILES} \
	ldbuildid.@OBJEXT@

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
ELF_DEPS = $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/elf-generic.em $(srcdir)/scripttempl/DWARF.sc
ELF_GEN_DEPS = $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/elf-generic.em $(srcdir)/emultempl/genelf.em $(srcdir)/scripttempl/DWARF.sc
ELF_X86_DEPS = $(ELF_DEPS) $(srcdir)/emulparams/plt_unwind.sh \
	       $(srcdir)/emulparams/extern_protected_data.sh \
	       $(srcdir)/emulparams/dynamic_undefined_weak.sh \
	       $(srcdir)/emulparams/reloc_overflow.sh \
	       $(srcdir)/emulparams/call_nop.sh

@TDIRS@

# We can't use pattern rules as we don't want to depend on GNU
# make, or else these rules could have been expressed in one
# two-liner: 'e%.c:' and '	${GENSCRIPTS} $* "$(tdir_$*)"'.
# (The recursive variable expansion is portable.)

run-genscripts:
	${GENSCRIPTS} $(script_target) "$($(script_tdirname))"

.PHONY: run-genscripts

$(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES):
	base=`echo $@ | sed -e 's,e\(.*\).c,\1,'`; \
	$(MAKE) run-genscripts "script_target=$$base" "script_tdirname=tdir_$$base"

eaix5ppc.c: $(srcdir)/emulparams/aix5ppc.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}

eaix5rs6.c: $(srcdir)/emulparams/aix5rs6.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}

eaixppc.c: $(srcdir)/emulparams/aixppc.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}

eaixrs6.c: $(srcdir)/emulparams/aixrs6.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}

ealpha.c: $(srcdir)/emulparams/alpha.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/alpha.sc ${GEN_DEPENDS}

ealphavms.c: $(srcdir)/emulparams/alphavms.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/vms.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/alphavms.sc ${GEN_DEPENDS}

earcv2elf.c: $(srcdir)/emulparams/arcv2elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfarcv2.sc ${GEN_DEPENDS}

earcv2elfx.c: $(srcdir)/emulparams/arcv2elfx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfarcv2.sc ${GEN_DEPENDS}

earcelf.c: $(srcdir)/emulparams/arcelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfarc.sc ${GEN_DEPENDS}

earcelf_prof.c: $(srcdir)/emulparams/arcelf_prof.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfarc.sc ${GEN_DEPENDS}

#for linux on arc
earclinux.c: $(srcdir)/emulparams/arclinux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/arclinux.em \
  $(srcdir)/scripttempl/arclinux.sc ${GEN_DEPENDS}

earclinux_prof.c: $(srcdir)/emulparams/arclinux_prof.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/arclinux.em \
  $(srcdir)/scripttempl/arclinux.sc ${GEN_DEPENDS}

earm_epoc_pe.c: $(srcdir)/emulparams/arm_epoc_pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/epocpe.sc ${GEN_DEPENDS}

earm_wince_pe.c: $(srcdir)/emulparams/arm_wince_pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

earmaoutb.c:	$(srcdir)/emulparams/armaoutb.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/armaout.sc ${GEN_DEPENDS}

earmaoutl.c:	$(srcdir)/emulparams/armaoutl.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/armaout.sc ${GEN_DEPENDS}

earmcoff.c:	$(srcdir)/emulparams/armcoff.sh \
  $(srcdir)/emultempl/armcoff.em $(srcdir)/scripttempl/armcoff.sc ${GEN_DEPENDS}

earmelf.c: $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_fbsd.c: $(srcdir)/emulparams/armelf_fbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_linux.c: $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_linux_eabi.c: $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_nacl.c: $(srcdir)/emulparams/armelf_nacl.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_nbsd.c: $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_nbsd_eabi.c: $(srcdir)/emulparams/armelf_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_nbsd_eabihf.c: $(srcdir)/emulparams/armelf_nbsd_eabihf.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_phoenix.c: $(srcdir)/emulparams/armelf_phoenix.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelf_vxworks.c: $(srcdir)/emulparams/armelf_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/vxworks.em \
  $(srcdir)/emultempl/armelf.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

earmelfb.c: $(srcdir)/emulparams/armelfb.sh $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_fbsd.c: $(srcdir)/emulparams/armelfb_fbsd.sh \
  $(srcdir)/emulparams/armelf_fbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_linux.c: $(srcdir)/emulparams/armelfb_linux.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_linux_eabi.c: $(srcdir)/emulparams/armelfb_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_nacl.c: $(srcdir)/emulparams/armelfb_nacl.sh \
  $(srcdir)/emulparams/armelf_nacl.sh \
  $(srcdir)/emulparams/armelf_linux_eabi.sh \
  $(srcdir)/emulparams/armelf_linux.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_nbsd.c: $(srcdir)/emulparams/armelfb_nbsd.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_nbsd_eabi.c: $(srcdir)/emulparams/armelfb_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmelfb_nbsd_eabihf.c: $(srcdir)/emulparams/armelfb_nbsd_eabihf.sh \
  $(srcdir)/emulparams/armelf_nbsd_eabi.sh \
  $(srcdir)/emulparams/armelf_nbsd.sh \
  $(srcdir)/emulparams/armelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmnbsd.c:	$(srcdir)/emulparams/armnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

earmnto.c: $(srcdir)/emulparams/armnto.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/armelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

earmpe.c: $(srcdir)/emulparams/armpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

earmsymbian.c: $(srcdir)/emulparams/armsymbian.sh \
  $(srcdir)/emulparams/armelf.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/armelf.em $(srcdir)/scripttempl/armbpabi.sc \
  ${GEN_DEPENDS}

eavr1.c: $(srcdir)/emulparams/avr1.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr2.c: $(srcdir)/emulparams/avr2.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr25.c: $(srcdir)/emulparams/avr25.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr3.c: $(srcdir)/emulparams/avr3.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr31.c: $(srcdir)/emulparams/avr31.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr35.c: $(srcdir)/emulparams/avr35.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr4.c: $(srcdir)/emulparams/avr4.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr5.c: $(srcdir)/emulparams/avr5.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr51.c: $(srcdir)/emulparams/avr51.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavr6.c: $(srcdir)/emulparams/avr6.sh $(srcdir)/emultempl/avrelf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega1.c: $(srcdir)/emulparams/avrxmega1.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega2.c: $(srcdir)/emulparams/avrxmega2.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega3.c: $(srcdir)/emulparams/avrxmega3.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega4.c: $(srcdir)/emulparams/avrxmega4.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega5.c: $(srcdir)/emulparams/avrxmega5.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega6.c: $(srcdir)/emulparams/avrxmega6.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrxmega7.c: $(srcdir)/emulparams/avrxmega7.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avr.sc \
  ${GEN_DEPENDS}

eavrtiny.c: $(srcdir)/emulparams/avrtiny.sh \
  $(srcdir)/emultempl/avrelf.em $(ELF_DEPS) $(srcdir)/scripttempl/avrtiny.sc \
  ${GEN_DEPENDS}

ecoff_i860.c: $(srcdir)/emulparams/coff_i860.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i860coff.sc ${GEN_DEPENDS}

ecoff_sparc.c: $(srcdir)/emulparams/coff_sparc.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sparccoff.sc ${GEN_DEPENDS}

ecrisaout.c: $(srcdir)/emulparams/crisaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/crisaout.sc ${GEN_DEPENDS}

ecriself.c: $(srcdir)/emulparams/criself.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ecrislinux.c: $(srcdir)/emulparams/crislinux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ed10velf.c: $(srcdir)/emulparams/d10velf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfd10v.sc ${GEN_DEPENDS}

ed30v_e.c: $(srcdir)/emulparams/d30v_e.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}

ed30v_o.c: $(srcdir)/emulparams/d30v_o.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}

ed30velf.c: $(srcdir)/emulparams/d30velf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elfd30v.sc ${GEN_DEPENDS}

edelta68.c: $(srcdir)/emulparams/delta68.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/delta68.sc ${GEN_DEPENDS}

eelf32_dlx.c: $(srcdir)/emulparams/elf32_dlx.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/dlx.sc ${GEN_DEPENDS}

eelf32_i860.c: $(srcdir)/emulparams/elf32_i860.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_i960.c: $(srcdir)/emulparams/elf32_i960.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_sparc.c: $(srcdir)/emulparams/elf32_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_sparc_sol2.c: $(srcdir)/emulparams/elf32_sparc_sol2.sh \
  $(srcdir)/emulparams/elf32_sparc.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_sparc_vxworks.c: $(srcdir)/emulparams/elf32_sparc_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emulparams/elf32_sparc.sh \
  $(srcdir)/emultempl/vxworks.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_spu.c: $(srcdir)/emulparams/elf32_spu.sh $(srcdir)/emultempl/spuelf.em \
  $(srcdir)/emultempl/spu_ovl.@OBJEXT@_c $(srcdir)/emultempl/spu_icache.@OBJEXT@_c \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

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

eelf32_tic6x_elf_be.c: $(srcdir)/emulparams/elf32_tic6x_elf_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}

eelf32_tic6x_elf_le.c: $(srcdir)/emulparams/elf32_tic6x_elf_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}

eelf32_tic6x_le.c: $(srcdir)/emulparams/elf32_tic6x_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}

eelf32_tic6x_linux_be.c: $(srcdir)/emulparams/elf32_tic6x_linux_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}

eelf32_tic6x_linux_le.c: $(srcdir)/emulparams/elf32_tic6x_linux_le.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc  $(srcdir)/emultempl/tic6xdsbt.em \
  ${GEN_DEPENDS}

eelf32am33lin.c: $(srcdir)/emulparams/elf32am33lin.sh \
  $(srcdir)/emulparams/elf32am33lin.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32b4300.c: $(srcdir)/emulparams/elf32b4300.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32bfin.c: $(srcdir)/emulparams/elf32bfin.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/bfin.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32bfinfd.c: $(srcdir)/emulparams/elf32bfinfd.sh \
  $(srcdir)/emulparams/elf32bfin.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/bfin.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32bmip.c: $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32bmipn32.c: $(srcdir)/emulparams/elf32bmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/irix.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32bsmip.c: $(srcdir)/emulparams/elf32bsmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) $(srcdir)/emultempl/irix.em \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32btsmip.c: $(srcdir)/emulparams/elf32btsmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32btsmip_fbsd.c: $(srcdir)/emulparams/elf32btsmip_fbsd.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32btsmipn32.c: $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32btsmipn32_fbsd.c: $(srcdir)/emulparams/elf32btsmipn32_fbsd.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32cr16.c: $(srcdir)/emulparams/elf32cr16.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/cr16elf.em \
  $(srcdir)/scripttempl/elf32cr16.sc ${GEN_DEPENDS}

eelf32cr16c.c: $(srcdir)/emulparams/elf32cr16c.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf32cr16c.sc ${GEN_DEPENDS}

eelf32crx.c: $(srcdir)/emulparams/elf32crx.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/crxelf.em \
  $(srcdir)/scripttempl/elf32crx.sc ${GEN_DEPENDS}

eelf32ebmip.c: $(srcdir)/emulparams/elf32ebmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ebmipvxworks.c: $(srcdir)/emulparams/elf32ebmipvxworks.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/emultempl/vxworks.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32elmip.c: $(srcdir)/emulparams/elf32elmip.sh \
  $(srcdir)/emulparams/elf32lmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32elmipvxworks.c: $(srcdir)/emulparams/elf32elmipvxworks.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/emultempl/vxworks.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lr5900.c: $(srcdir)/emulparams/elf32lr5900.sh \
  $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32lr5900n32.c: $(srcdir)/emulparams/elf32lr5900n32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32epiphany.c: $(srcdir)/emulparams/elf32epiphany.sh \
  $(ELF_DEPS) ${GEN_DEPENDS}

eelf32epiphany_4x4.c: $(srcdir)/emulparams/elf32epiphany_4x4.sh \
  $(srcdir)/emultempl/elf32.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/epiphany_4x4.sc ${GEN_DEPENDS}

eelf32fr30.c: $(srcdir)/emulparams/elf32fr30.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32frv.c: $(srcdir)/emulparams/elf32frv.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32frvfd.c: $(srcdir)/emulparams/elf32frvfd.sh \
  $(srcdir)/emulparams/elf32frv.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ft32.c: $(srcdir)/emulparams/elf32ft32.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32i370.c: $(srcdir)/emulparams/elf32i370.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfi370.sc ${GEN_DEPENDS}

eelf32ip2k.c: $(srcdir)/emulparams/elf32ip2k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32iq10.c: $(srcdir)/emulparams/elf32iq10.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/iq2000.sc ${GEN_DEPENDS}

eelf32iq2000.c: $(srcdir)/emulparams/elf32iq2000.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/iq2000.sc ${GEN_DEPENDS}

eelf32l4300.c: $(srcdir)/emulparams/elf32l4300.sh \
  $(srcdir)/emulparams/elf32b4300.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32lm32.c: $(srcdir)/emulparams/elf32lm32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lm32fd.c: $(srcdir)/emulparams/elf32lm32fd.sh \
  $(srcdir)/emulparams/elf32lm32.sh $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lmip.c: $(srcdir)/emulparams/elf32lmip.sh \
  $(srcdir)/emulparams/elf32bmip.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lppc.c: $(srcdir)/emulparams/elf32lppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lppclinux.c: $(srcdir)/emulparams/elf32lppclinux.sh \
  $(srcdir)/emulparams/elf32lppc.sh $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lppcnto.c: $(srcdir)/emulparams/elf32lppcnto.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lppcsim.c: $(srcdir)/emulparams/elf32lppcsim.sh \
  $(srcdir)/emulparams/elf32lppc.sh $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lriscv.c: $(srcdir)/emulparams/elf32lriscv.sh \
  $(srcdir)/emulparams/elf32lriscv-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/riscvelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32lsmip.c: $(srcdir)/emulparams/elf32lsmip.sh \
  $(srcdir)/emulparams/elf32lmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32ltsmip.c: $(srcdir)/emulparams/elf32ltsmip.sh \
  $(srcdir)/emulparams/elf32btsmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32ltsmip_fbsd.c: $(srcdir)/emulparams/elf32ltsmip_fbsd.sh \
  $(srcdir)/emulparams/elf32btsmip.sh $(srcdir)/emulparams/elf32bmip.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc \
  ${GEN_DEPENDS}

eelf32ltsmipn32.c: $(srcdir)/emulparams/elf32ltsmipn32.sh \
  $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ltsmipn32_fbsd.c: $(srcdir)/emulparams/elf32ltsmipn32_fbsd.sh \
  $(srcdir)/emulparams/elf32btsmipn32.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32m32c.c: $(srcdir)/emulparams/elf32m32c.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32mbel_linux.c: $(srcdir)/emulparams/elf32mbel_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32mb_linux.c: $(srcdir)/emulparams/elf32mb_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32mcore.c: $(srcdir)/emulparams/elf32mcore.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32mep.c: $(srcdir)/emulparams/elf32mep.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/mep.sc ${GEN_DEPENDS}

eelf32metag.c: $(srcdir)/emulparams/elf32metag.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/metagelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32microblazeel.c: $(srcdir)/emulparams/elf32microblazeel.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfmicroblaze.sc ${GEN_DEPENDS}

eelf32microblaze.c: $(srcdir)/emulparams/elf32microblaze.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elfmicroblaze.sc ${GEN_DEPENDS}

eelf32mipswindiss.c: $(srcdir)/emulparams/elf32mipswindiss.sh $(ELF_DEPS) \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32moxie.c: $(srcdir)/emulparams/elf32moxie.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

emoxiebox.c: $(srcdir)/emulparams/moxiebox.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32mt.c: $(srcdir)/emulparams/elf32mt.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32or1k.c: $(srcdir)/emulparams/elf32or1k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32or1k_linux.c: $(srcdir)/emulparams/elf32or1k_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32or1k_nbsd.c: $(srcdir)/emulparams/elf32or1k_nbsd.sh \
  $(srcdir)/emulparams/elf32or1k.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppc.c: $(srcdir)/emulparams/elf32ppc.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppc_fbsd.c: $(srcdir)/emulparams/elf32ppc_fbsd.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppc_nbsd.c: $(srcdir)/emulparams/elf32ppc_nbsd.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em $(ELF_DEPS) \
  ldemul-list.h \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppclinux.c: $(srcdir)/emulparams/elf32ppclinux.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppcnto.c: $(srcdir)/emulparams/elf32ppcnto.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppcsim.c: $(srcdir)/emulparams/elf32ppcsim.sh \
  $(srcdir)/emulparams/elf32ppc.sh $(srcdir)/emulparams/elf32ppccommon.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppcvxworks.c: $(srcdir)/emulparams/elf32ppcvxworks.sh \
  $(srcdir)/emulparams/elf32ppccommon.sh $(srcdir)/emulparams/vxworks.sh \
  $(srcdir)/emultempl/ppc32elf.em ldemul-list.h \
  $(srcdir)/emultempl/vxworks.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32ppcwindiss.c: $(srcdir)/emulparams/elf32ppcwindiss.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32rl78.c: $(srcdir)/emulparams/elf32rl78.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32rx.c: $(srcdir)/emulparams/elf32rx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32tilegx.c: $(srcdir)/emulparams/elf32tilegx.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32tilegx_be.c: $(srcdir)/emulparams/elf32tilegx_be.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32tilepro.c: $(srcdir)/emulparams/elf32tilepro.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32vax.c: $(srcdir)/emulparams/elf32vax.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32visium.c: $(srcdir)/emulparams/elf32visium.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/visium.sc ${GEN_DEPENDS}

eelf32xc16x.c: $(srcdir)/emulparams/elf32xc16x.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32xc16xl.c: $(srcdir)/emulparams/elf32xc16xl.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32xc16xs.c: $(srcdir)/emulparams/elf32xc16xs.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32xstormy16.c: $(srcdir)/emulparams/elf32xstormy16.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/xstormy16.sc ${GEN_DEPENDS}

eelf32xtensa.c: $(srcdir)/emulparams/elf32xtensa.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/xtensaelf.em $(INCDIR)/xtensa-config.h \
  $(BFDDIR)/elf-bfd.h $(BFDDIR)/libbfd.h $(INCDIR)/elf/xtensa.h \
  $(srcdir)/scripttempl/elfxtensa.sc ${GEN_DEPENDS}

eelf_i386.c: $(srcdir)/emulparams/elf_i386.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_be.c: $(srcdir)/emulparams/elf_i386_be.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_chaos.c: $(srcdir)/emulparams/elf_i386_chaos.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf_chaos.sc ${GEN_DEPENDS}

eelf_i386_fbsd.c: $(srcdir)/emulparams/elf_i386_fbsd.sh \
  $(srcdir)/emulparams/elf_i386.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_ldso.c: $(srcdir)/emulparams/elf_i386_ldso.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_nacl.c: $(srcdir)/emulparams/elf_i386_nacl.sh \
  $(srcdir)/emulparams/elf_i386.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_sol2.c: $(srcdir)/emulparams/elf_i386_sol2.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_i386_vxworks.c: $(srcdir)/emulparams/elf_i386_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(srcdir)/emultempl/vxworks.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_iamcu.c: $(srcdir)/emulparams/elf_iamcu.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_s390.c: $(srcdir)/emulparams/elf_s390.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

egld960.c: $(srcdir)/emulparams/gld960.sh \
  $(srcdir)/emultempl/gld960.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}

egld960coff.c: $(srcdir)/emulparams/gld960coff.sh \
  $(srcdir)/emultempl/gld960c.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}

eh8300.c: $(srcdir)/emulparams/h8300.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300.sc ${GEN_DEPENDS}

eh8300elf.c: $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300elf_linux.c: $(srcdir)/emulparams/h8300elf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300h.c: $(srcdir)/emulparams/h8300h.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300h.sc ${GEN_DEPENDS}

eh8300helf.c: $(srcdir)/emulparams/h8300helf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300helf_linux.c: $(srcdir)/emulparams/h8300helf_linux.sh \
  $(srcdir)/emulparams/h8300elf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300hn.c: $(srcdir)/emulparams/h8300hn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300hn.sc ${GEN_DEPENDS}

eh8300hnelf.c: $(srcdir)/emulparams/h8300hnelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300s.c: $(srcdir)/emulparams/h8300s.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300s.sc ${GEN_DEPENDS}

eh8300self.c: $(srcdir)/emulparams/h8300self.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300self_linux.c: $(srcdir)/emulparams/h8300self_linux.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300sn.c: $(srcdir)/emulparams/h8300sn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sn.sc ${GEN_DEPENDS}

eh8300snelf.c: $(srcdir)/emulparams/h8300snelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300sx.c: $(srcdir)/emulparams/h8300sx.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sx.sc ${GEN_DEPENDS}

eh8300sxelf.c: $(srcdir)/emulparams/h8300sxelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300sxelf_linux.c: $(srcdir)/emulparams/h8300sxelf_linux.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8300sxn.c: $(srcdir)/emulparams/h8300sxn.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8300sxn.sc ${GEN_DEPENDS}

eh8300sxnelf.c: $(srcdir)/emulparams/h8300sxnelf.sh \
  $(srcdir)/emulparams/h8300elf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eh8500.c: $(srcdir)/emulparams/h8500.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500.sc ${GEN_DEPENDS}

eh8500b.c: $(srcdir)/emulparams/h8500b.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500b.sc ${GEN_DEPENDS}

eh8500c.c: $(srcdir)/emulparams/h8500c.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500c.sc ${GEN_DEPENDS}

eh8500m.c: $(srcdir)/emulparams/h8500m.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500m.sc ${GEN_DEPENDS}

eh8500s.c: $(srcdir)/emulparams/h8500s.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/h8500s.sc ${GEN_DEPENDS}

ehp300bsd.c: $(srcdir)/emulparams/hp300bsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ehp3hpux.c: $(srcdir)/emulparams/hp3hpux.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ehppaelf.c: $(srcdir)/emulparams/hppaelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/hppaelf.sc ${GEN_DEPENDS}

ehppalinux.c: $(srcdir)/emulparams/hppalinux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ehppanbsd.c: $(srcdir)/emulparams/hppanbsd.sh \
  $(srcdir)/emulparams/hppaelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ehppaobsd.c: $(srcdir)/emulparams/hppaobsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/hppaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ei386aout.c: $(srcdir)/emulparams/i386aout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ei386beos.c: $(srcdir)/emulparams/i386beos.sh \
  $(srcdir)/emultempl/beos.em $(srcdir)/scripttempl/i386beos.sc ${GEN_DEPENDS}

ei386bsd.c: $(srcdir)/emulparams/i386bsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ei386coff.c: $(srcdir)/emulparams/i386coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386coff.sc ${GEN_DEPENDS}

ei386go32.c: $(srcdir)/emulparams/i386go32.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386go32.sc ${GEN_DEPENDS}

ei386linux.c: $(srcdir)/emulparams/i386linux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ei386lynx.c: $(srcdir)/emulparams/i386lynx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ei386mach.c: $(srcdir)/emulparams/i386mach.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ei386moss.c: $(srcdir)/emulparams/i386moss.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ei386msdos.c: $(srcdir)/emulparams/i386msdos.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/i386msdos.sc ${GEN_DEPENDS}

ei386nbsd.c:	$(srcdir)/emulparams/i386nbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ei386nto.c:	$(srcdir)/emulparams/i386nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ei386nw.c:	$(srcdir)/emulparams/i386nw.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/nw.sc ${GEN_DEPENDS}

ei386pe.c: $(srcdir)/emulparams/i386pe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

ei386pe_posix.c: $(srcdir)/emulparams/i386pe_posix.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

ei386pep.c: $(srcdir)/emulparams/i386pep.sh \
  $(srcdir)/emultempl/pep.em $(srcdir)/scripttempl/pep.sc ${GEN_DEPENDS}

elnk960.c: $(srcdir)/emulparams/lnk960.sh \
  $(srcdir)/emultempl/lnk960.em $(srcdir)/scripttempl/i960.sc ${GEN_DEPENDS}

em32relf.c: $(srcdir)/emulparams/m32relf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em32relf_linux.c: $(srcdir)/emulparams/m32relf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em32rlelf.c: $(srcdir)/emulparams/m32rlelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em32rlelf_linux.c: $(srcdir)/emulparams/m32rlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em68hc11elf.c: $(srcdir)/emulparams/m68hc11elf.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc11.sc ${GEN_DEPENDS}

em68hc11elfb.c: $(srcdir)/emulparams/m68hc11elfb.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc11.sc ${GEN_DEPENDS}

em68hc12elf.c: $(srcdir)/emulparams/m68hc12elf.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc12.sc ${GEN_DEPENDS}

em68hc12elfb.c: $(srcdir)/emulparams/m68hc12elfb.sh \
  $(srcdir)/emultempl/m68hc1xelf.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfm68hc12.sc ${GEN_DEPENDS}

em68k4knbsd.c:	$(srcdir)/emulparams/m68k4knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

em68kaout.c: $(srcdir)/emulparams/m68kaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

em68kaux.c: $(srcdir)/emulparams/m68kaux.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/m68kaux.sc ${GEN_DEPENDS}

em68kcoff.c: $(srcdir)/emulparams/m68kcoff.sh \
  $(srcdir)/emultempl/m68kcoff.em $(srcdir)/scripttempl/m68kcoff.sc ${GEN_DEPENDS}

em68kelf.c: $(srcdir)/emulparams/m68kelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/m68kelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em68kelfnbsd.c: $(srcdir)/emulparams/m68kelfnbsd.sh \
  $(srcdir)/emulparams/m68kelf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/m68kelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

em68klinux.c: $(srcdir)/emulparams/m68klinux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

em68knbsd.c:	$(srcdir)/emulparams/m68knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

em68kpsos.c:	$(srcdir)/emulparams/m68kpsos.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/psos.sc ${GEN_DEPENDS}

em88kbcs.c: $(srcdir)/emulparams/m88kbcs.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/m88kbcs.sc ${GEN_DEPENDS}

emcorepe.c: $(srcdir)/emulparams/mcorepe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

emn10200.c: $(srcdir)/emulparams/mn10200.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

emn10300.c: $(srcdir)/emulparams/mn10300.sh \
  $(srcdir)/emulparams/mn10200.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

emsp430elf.c: $(srcdir)/emulparams/msp430elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/msp430.em \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc ${GEN_DEPENDS}

emsp430X.c: $(srcdir)/emulparams/msp430elf.sh $(srcdir)/emulparams/msp430X.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/msp430.em \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf32msp430.sc ${GEN_DEPENDS}

ends32elf.c: $(srcdir)/emulparams/nds32elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

ends32elf16m.c: $(srcdir)/emulparams/nds32elf16m.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

ends32belf.c: $(srcdir)/emulparams/nds32belf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

ends32belf16m.c: $(srcdir)/emulparams/nds32belf16m.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

ends32elf_linux.c: $(srcdir)/emulparams/nds32elf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

ends32belf_linux.c: $(srcdir)/emulparams/nds32belf_linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/nds32elf.em \
  $(srcdir)/scripttempl/nds32elf.sc ${GEN_DEPENDS}

enews.c: $(srcdir)/emulparams/news.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

enios2elf.c: $(srcdir)/emulparams/nios2elf.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/nios2elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

enios2linux.c: $(srcdir)/emulparams/nios2linux.sh \
  $(srcdir)/emultempl/elf32.em $(srcdir)/emultempl/nios2elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ens32knbsd.c:	$(srcdir)/emulparams/ns32knbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/netbsd.em \
  $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

eaarch64elf.c: $(srcdir)/emulparams/aarch64elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64elf32.c: $(srcdir)/emulparams/aarch64elf32.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64elfb.c: $(srcdir)/emulparams/aarch64elfb.sh $(srcdir)/emulparams/aarch64elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64elf32b.c: $(srcdir)/emulparams/aarch64elf32b.sh $(srcdir)/emulparams/aarch64elf32.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64cloudabi.c: $(srcdir)/emulparams/aarch64cloudabi.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64cloudabib.c: $(srcdir)/emulparams/aarch64cloudabib.sh $(srcdir)/emulparams/aarch64cloudabi.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64fbsd.c: $(srcdir)/emulparams/aarch64fbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64fbsdb.c: $(srcdir)/emulparams/aarch64fbsdb.sh $(srcdir)/emulparams/aarch64fbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64linux.c: $(srcdir)/emulparams/aarch64linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64linuxb.c: $(srcdir)/emulparams/aarch64linuxb.sh $(srcdir)/emulparams/aarch64linux.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64nbsd.c: $(srcdir)/emulparams/aarch64nbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64nbsdb.c: $(srcdir)/emulparams/aarch64nbsdb.sh $(srcdir)/emulparams/aarch64nbsd.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eor32.c: $(srcdir)/emulparams/or32.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/or32.sc ${GEN_DEPENDS}

eor32elf.c: $(srcdir)/emulparams/or32elf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} or32elf "$(tdir_or32elf)"

eaarch64linux32.c: $(srcdir)/emulparams/aarch64linux32.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eaarch64linux32b.c: $(srcdir)/emulparams/aarch64linux32b.sh $(srcdir)/emulparams/aarch64linux32.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/aarch64elf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

epc532macha.c: $(srcdir)/emulparams/pc532macha.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

epdp11.c: $(srcdir)/emulparams/pdp11.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

epjelf.c: $(srcdir)/emulparams/pjelf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

epjlelf.c: $(srcdir)/emulparams/pjlelf.sh $(srcdir)/emulparams/pjelf.sh \
  $(ELF_GEN_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eppclynx.c: $(srcdir)/emulparams/ppclynx.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eppcmacos.c:	$(srcdir)/emulparams/ppcmacos.sh \
  $(srcdir)/emultempl/aix.em $(srcdir)/scripttempl/aix.sc ${GEN_DEPENDS}

eppcnw.c:	$(srcdir)/emulparams/ppcnw.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/nw.sc ${GEN_DEPENDS}

eppcpe.c: $(srcdir)/emulparams/ppcpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/ppcpe.sc ${GEN_DEPENDS}

eriscix.c: $(srcdir)/emulparams/riscix.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

escore3_elf.c: $(srcdir)/emulparams/score3_elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/scoreelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

escore7_elf.c: $(srcdir)/emulparams/score3_elf.sh \
  $(srcdir)/emulparams/score7_elf.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/scoreelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

esh.c: $(srcdir)/emulparams/sh.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sh.sc ${GEN_DEPENDS}

eshelf.c: $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf32.c: $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf32_linux.c: $(srcdir)/emulparams/shelf32_linux.sh \
  $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf32_nbsd.c: $(srcdir)/emulparams/shelf32_nbsd.sh \
  $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_fd.c: $(srcdir)/emulparams/shelf_fd.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_linux.c: $(srcdir)/emulparams/shelf_linux.sh \
  $(srcdir)/emulparams/shlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_nbsd.c: $(srcdir)/emulparams/shelf_nbsd.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_nto.c: $(srcdir)/emulparams/shelf_nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_uclinux.c: $(srcdir)/emulparams/shelf_uclinux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf_vxworks.c: $(srcdir)/emulparams/shelf_vxworks.sh \
  $(srcdir)/emulparams/vxworks.sh $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc \
  $(srcdir)/emultempl/vxworks.em ${GEN_DEPENDS}

eshl.c: $(srcdir)/emulparams/shl.sh \
  $(srcdir)/emulparams/sh.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/sh.sc ${GEN_DEPENDS}

eshlelf.c: $(srcdir)/emulparams/shlelf.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf32.c: $(srcdir)/emulparams/shlelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h $(srcdir)/emulparams/shelf32.sh \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf32_linux.c: $(srcdir)/emulparams/shlelf32_linux.sh \
  $(srcdir)/emulparams/shelf32_linux.sh $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf32_nbsd.c: $(srcdir)/emulparams/shlelf32_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(BFDDIR)/libbfd.h $(INCDIR)/libiberty.h \
  $(srcdir)/emultempl/sh64elf.em $(INCDIR)/elf/sh.h $(BFDDIR)/elf-bfd.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf_fd.c: $(srcdir)/emulparams/shlelf_fd.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf_linux.c: $(srcdir)/emulparams/shlelf_linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf_nbsd.c: $(srcdir)/emulparams/shlelf_nbsd.sh \
  $(srcdir)/emulparams/shelf_nbsd.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf_nto.c: $(srcdir)/emulparams/shlelf_nto.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf_vxworks.c: $(srcdir)/emulparams/shlelf_vxworks.sh \
  $(srcdir)/emulparams/shelf_vxworks.sh $(srcdir)/emulparams/vxworks.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc $(srcdir)/emultempl/vxworks.em \
  ${GEN_DEPENDS}

eshlsymbian.c: $(srcdir)/emulparams/shlsymbian.sh \
  $(srcdir)/emulparams/shelf.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf32sh-symbian.sc ${GEN_DEPENDS}

eshpe.c: $(srcdir)/emulparams/shpe.sh \
  $(srcdir)/emultempl/pe.em $(srcdir)/scripttempl/pe.sc ${GEN_DEPENDS}

esparcaout.c: $(srcdir)/emulparams/sparcaout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

esparclinux.c: $(srcdir)/emulparams/sparclinux.sh \
  $(srcdir)/emultempl/linux.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

esparcnbsd.c:	$(srcdir)/emulparams/sparcnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

est2000.c: $(srcdir)/emulparams/st2000.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/st2000.sc ${GEN_DEPENDS}

esun3.c: $(srcdir)/emulparams/sun3.sh \
  $(srcdir)/emultempl/sunos.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

esun4.c: $(srcdir)/emulparams/sun4.sh \
  $(srcdir)/emultempl/sunos.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

etic30aout.c: $(srcdir)/emulparams/tic30aout.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic30aout.sc ${GEN_DEPENDS}

etic30coff.c: $(srcdir)/emulparams/tic30coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic30coff.sc ${GEN_DEPENDS}

etic3xcoff.c: $(srcdir)/emulparams/tic3xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}

etic3xcoff_onchip.c: $(srcdir)/emulparams/tic3xcoff_onchip.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}

etic4xcoff.c: $(srcdir)/emulparams/tic4xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic4xcoff.sc ${GEN_DEPENDS}

etic54xcoff.c: $(srcdir)/emulparams/tic54xcoff.sh \
  $(srcdir)/emultempl/ticoff.em $(srcdir)/scripttempl/tic54xcoff.sc ${GEN_DEPENDS}

etic80coff.c: $(srcdir)/emulparams/tic80coff.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/tic80coff.sc ${GEN_DEPENDS}

ev850.c: $(srcdir)/emulparams/v850.sh $(srcdir)/emultempl/v850elf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/v850.sc ${GEN_DEPENDS}

ev850_rh850.c: $(srcdir)/emulparams/v850_rh850.sh $(srcdir)/emultempl/v850elf.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/v850_rh850.sc ${GEN_DEPENDS}

evanilla.c: $(srcdir)/emulparams/vanilla.sh \
  $(srcdir)/emultempl/vanilla.em $(srcdir)/scripttempl/vanilla.sc ${GEN_DEPENDS}

evax.c: $(srcdir)/emulparams/vax.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

evaxnbsd.c: $(srcdir)/emulparams/vaxnbsd.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

evsta.c: $(srcdir)/emulparams/vsta.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/aout.sc ${GEN_DEPENDS}

ew65.c: $(srcdir)/emulparams/w65.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/w65.sc ${GEN_DEPENDS}

exgateelf.c: $(srcdir)/emulparams/xgateelf.sh \
  $(srcdir)/emultempl/generic.em $(ELF_DEPS) \
  $(srcdir)/scripttempl/elfxgate.sc ${GEN_DEPENDS}

ez80.c: $(srcdir)/emulparams/z80.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/z80.em \
  $(srcdir)/scripttempl/z80.sc ${GEN_DEPENDS}

ez8001.c: $(srcdir)/emulparams/z8001.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/z8000.sc ${GEN_DEPENDS}

ez8002.c: $(srcdir)/emulparams/z8002.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/scripttempl/z8000.sc ${GEN_DEPENDS}

eelf32_x86_64.c: $(srcdir)/emulparams/elf32_x86_64.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf32_x86_64_nacl.c: $(srcdir)/emulparams/elf32_x86_64_nacl.sh \
  $(srcdir)/emulparams/elf32_x86_64.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_aix.c: $(srcdir)/emulparams/elf64_aix.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_ia64.c: $(srcdir)/emulparams/elf64_ia64.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/ia64elf.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_ia64_fbsd.c: $(srcdir)/emulparams/elf64_ia64_fbsd.sh \
  $(srcdir)/emulparams/elf64_ia64.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/ia64elf.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_ia64_vms.c: $(srcdir)/emulparams/elf64_ia64_vms.sh \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/vms.em \
  $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/ia64vms.sc ${GEN_DEPENDS}

eelf64_s390.c: $(srcdir)/emulparams/elf64_s390.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_sparc.c: $(srcdir)/emulparams/elf64_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_sparc_fbsd.c: $(srcdir)/emulparams/elf64_sparc_fbsd.sh \
  $(srcdir)/emulparams/elf64_sparc.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64_sparc_sol2.c: $(srcdir)/emulparams/elf64_sparc_sol2.sh \
  $(srcdir)/emulparams/elf64_sparc.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64alpha.c: $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64alpha_fbsd.c: $(srcdir)/emulparams/elf64alpha_fbsd.sh \
  $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64alpha_nbsd.c: $(srcdir)/emulparams/elf64alpha_nbsd.sh \
  $(srcdir)/emulparams/elf64alpha.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/alphaelf.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64bmip.c: $(srcdir)/emulparams/elf64bmip.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/irix.em $(srcdir)/emultempl/mipself.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64btsmip.c: $(srcdir)/emulparams/elf64btsmip.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64btsmip_fbsd.c: $(srcdir)/emulparams/elf64btsmip_fbsd.sh \
  $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64hppa.c: $(srcdir)/emulparams/elf64hppa.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf64hppa.sc ${GEN_DEPENDS}

eelf64lppc.c: $(srcdir)/emulparams/elf64lppc.sh \
  $(srcdir)/emulparams/elf64ppc.sh $(srcdir)/emultempl/ppc64elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64lriscv.c: $(srcdir)/emulparams/elf64lriscv.sh \
  $(srcdir)/emulparams/elf64lriscv-defs.sh \
  $(srcdir)/emulparams/elf32lriscv-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/riscvelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}
	${GENSCRIPTS} elf64lriscv "$(tdir_elf64lriscv)"

eelf64ltsmip.c: $(srcdir)/emulparams/elf64ltsmip.sh \
  $(srcdir)/emulparams/elf64btsmip.sh $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64ltsmip_fbsd.c: $(srcdir)/emulparams/elf64ltsmip_fbsd.sh \
  $(srcdir)/emulparams/elf64btsmip_fbsd.sh $(srcdir)/emulparams/elf64bmip-defs.sh \
  $(srcdir)/emulparams/elf32bmipn32-defs.sh $(ELF_DEPS) \
  $(srcdir)/emultempl/mipself.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64mmix.c: $(srcdir)/emulparams/elf64mmix.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/mmix-elfnmmo.em \
  $(srcdir)/emultempl/mmixelf.em $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64ppc.c: $(srcdir)/emulparams/elf64ppc.sh $(srcdir)/emultempl/ppc64elf.em \
  ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64ppc_fbsd.c: $(srcdir)/emulparams/elf64ppc_fbsd.sh \
  $(srcdir)/emultempl/ppc64elf.em ldemul-list.h \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64rdos.c: $(srcdir)/emulparams/elf64rdos.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64tilegx.c: $(srcdir)/emulparams/elf64tilegx.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf64tilegx_be.c: $(srcdir)/emulparams/elf64tilegx_be.sh \
  $(ELF_DEPS) $(srcdir)/emultempl/needrelax.em \
  $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_l1om.c: $(srcdir)/emulparams/elf_l1om.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_l1om_fbsd.c: $(srcdir)/emulparams/elf_l1om_fbsd.sh \
  $(srcdir)/emulparams/elf_l1om.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_k1om.c: $(srcdir)/emulparams/elf_k1om.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_k1om_fbsd.c: $(srcdir)/emulparams/elf_k1om_fbsd.sh \
  $(srcdir)/emulparams/elf_k1om.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_x86_64.c: $(srcdir)/emulparams/elf_x86_64.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_x86_64_cloudabi.c: $(srcdir)/emulparams/elf_x86_64_cloudabi.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_x86_64_fbsd.c: $(srcdir)/emulparams/elf_x86_64_fbsd.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_x86_64_nacl.c: $(srcdir)/emulparams/elf_x86_64_nacl.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(srcdir)/emulparams/elf_nacl.sh \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eelf_x86_64_sol2.c: $(srcdir)/emulparams/elf_x86_64_sol2.sh \
  $(srcdir)/emulparams/elf_x86_64.sh \
  $(srcdir)/emulparams/solaris2.sh \
  $(srcdir)/emultempl/solaris2.em \
  $(ELF_X86_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

ehppa64linux.c: $(srcdir)/emulparams/hppa64linux.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

emmo.c: $(srcdir)/emulparams/mmo.sh $(srcdir)/emultempl/mmix-elfnmmo.em \
  $(srcdir)/emultempl/generic.em $(srcdir)/emultempl/elf-generic.em \
  $(srcdir)/emultempl/mmo.em $(srcdir)/scripttempl/DWARF.sc \
  $(srcdir)/scripttempl/mmo.sc ${GEN_DEPENDS}

eshelf64.c: $(srcdir)/emulparams/shelf64.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshelf64_nbsd.c: $(srcdir)/emulparams/shelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf64.c: $(srcdir)/emulparams/shlelf64.sh \
  $(srcdir)/emulparams/shelf64.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}

eshlelf64_nbsd.c: $(srcdir)/emulparams/shlelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf64_nbsd.sh \
  $(srcdir)/emulparams/shelf32_nbsd.sh $(srcdir)/emulparams/shelf32.sh \
  $(ELF_DEPS) $(srcdir)/scripttempl/elf.sc ${GEN_DEPENDS}


# We need this for automake to use YLWRAP.
EXTRA_ld_new_SOURCES = deffilep.y ldlex.l
# Allow dependency tracking to work for these files, too.
EXTRA_ld_new_SOURCES += pep-dll.c pe-dll.c

ld_new_SOURCES = ldgram.y ldlex-wrapper.c lexsup.c ldlang.c mri.c ldctor.c ldmain.c \
	ldwrite.c ldexp.c ldemul.c ldver.c ldmisc.c ldfile.c ldcref.c $(PLUGIN_C) \
	ldbuildid.c
ld_new_DEPENDENCIES = $(EMULATION_OFILES) $(EMUL_EXTRA_OFILES) \
		      $(BFDLIB) $(LIBIBERTY) $(LIBINTL_DEP)
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
noinst_LTLIBRARIES = libldtestplug.la libldtestplug2.la \
		     libldtestplug3.la libldtestplug4.la
libldtestplug_la_SOURCES = testplug.c
libldtestplug_la_CFLAGS= -g -O2
libldtestplug_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug2_la_SOURCES = testplug2.c
libldtestplug2_la_CFLAGS= -g -O2
libldtestplug2_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug3_la_SOURCES = testplug3.c
libldtestplug3_la_CFLAGS= -g -O2
libldtestplug3_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug4_la_SOURCES = testplug4.c
libldtestplug4_la_CFLAGS= -g -O2
libldtestplug4_la_LDFLAGS = -no-undefined -rpath /nowhere
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

# We want to reconfigure if configure.host or configure.tgt changes.
# development.sh is used to determine -Werror default.
CONFIG_STATUS_DEPENDENCIES = $(srcdir)/configure.host $(srcdir)/configure.tgt \
	$(BFDDIR)/development.sh

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
	  || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	fi; \
	if test "x$(install_as_default)" = "xyes"; then \
	  ld=`echo ld | sed '$(transform)'`; \
	  rm -f $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  if test "$(bindir)" != "$(tooldir)/bin"; then \
	    rm -f $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	    ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT) >/dev/null 2>/dev/null \
	    || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	  fi; \
	fi

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(scriptdir)/ldscripts
	for f in ldscripts/* ; do \
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
