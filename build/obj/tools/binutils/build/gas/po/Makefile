# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995, 1996, 1997 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
# Copyright (C) 2003-2022 Free Software Foundation, Inc.
#
# This file may be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

PACKAGE = gas
VERSION = 2.39

SHELL = /bin/sh


srcdir = /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/po
top_srcdir = /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas
VPATH = /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/po
top_builddir = ..

prefix = /Users/sidqian/Downloads/summer/L2S/netbsd-src/build/obj/tooldir.Darwin-22.1.0-x86_64
exec_prefix = ${prefix}
datadir = ${datarootdir}
localedir = ${datarootdir}/locale
gnulocaledir = ${prefix}/share/locale
gettextsrcdir = ${prefix}/share/gettext/po
subdir = po

DESTDIR =

INSTALL = /Users/sidqian/Downloads/summer/L2S/netbsd-src/build/obj/tooldir.Darwin-22.1.0-x86_64/bin/m68k--netbsdelf-install -c  -r
INSTALL_DATA = ${INSTALL} -m 644
MKINSTALLDIRS = /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/../mkinstalldirs

CC = cc
GENCAT = 
GMSGFMT = PATH=../src:$$PATH /usr/local/bin/msgfmt
MSGFMT = /usr/local/bin/msgfmt
XGETTEXT = PATH=../src:$$PATH /usr/local/bin/xgettext
MSGMERGE = PATH=../src:$$PATH msgmerge

DEFS = -DHAVE_CONFIG_H
CFLAGS = -O -O2 -no-cpp-precomp  
CPPFLAGS = 

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

SOURCES = cat-id-tbl.c
POFILES =  $(srcdir)/es.po $(srcdir)/fi.po $(srcdir)/fr.po $(srcdir)/id.po $(srcdir)/ja.po $(srcdir)/ru.po $(srcdir)/rw.po $(srcdir)/sv.po $(srcdir)/tr.po $(srcdir)/uk.po $(srcdir)/zh_CN.po
GMOFILES =  $(srcdir)/es.gmo $(srcdir)/fi.gmo $(srcdir)/fr.gmo $(srcdir)/id.gmo $(srcdir)/ja.gmo $(srcdir)/ru.gmo $(srcdir)/rw.gmo $(srcdir)/sv.gmo $(srcdir)/tr.gmo $(srcdir)/uk.gmo $(srcdir)/zh_CN.gmo
DISTFILES = ChangeLog Makefile.in.in POTFILES.in $(PACKAGE).pot \
stamp-cat-id $(POFILES) $(GMOFILES) $(SOURCES)

POTFILES = \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/app.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/as.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/as.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/asintl.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/atof-generic.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/bignum.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/bit_fix.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/cgen.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/cgen.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/compress-debug.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/compress-debug.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/cond.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/atof-ieee.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/atof-vax.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/bfin-lex-wrapper.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-crisaout.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-criself.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-i386aout.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-i386coff.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-i386elf.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/e-mipself.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-aout.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-aout.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-coff.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-coff.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-ecoff.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-ecoff.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-elf.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-elf.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-evax.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-evax.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-fdpicelf.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-fdpicelf.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-macho.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-macho.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-multi.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-multi.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-som.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/obj-som.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-aarch64.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-aarch64.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-alpha.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-alpha.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-arc.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-arc.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-arm.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-arm.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-avr.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-avr.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-bfin.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-bfin.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-bpf.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-bpf.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-cr16.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-cr16.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-cris.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-cris.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-crx.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-crx.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-csky.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-csky.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-d10v.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-d10v.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-d30v.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-d30v.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-dlx.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-dlx.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-epiphany.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-epiphany.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-fr30.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-fr30.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-frv.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-frv.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ft32.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ft32.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-h8300.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-h8300.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-hppa.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-hppa.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-i386.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-i386.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ia64.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ia64.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ip2k.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ip2k.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-iq2000.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-iq2000.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-lm32.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-lm32.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-loongarch.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m32c.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m32c.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m32r.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m32r.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m68hc11.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m68hc11.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m68k.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-m68k.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mcore.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mcore.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mep.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mep.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-metag.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-metag.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-microblaze.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-microblaze.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mips.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mips.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mmix.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mmix.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mn10200.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mn10200.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mn10300.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mn10300.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-moxie.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-msp430.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-msp430.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mt.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-mt.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-nds32.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-nds32.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-nios2.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-nios2.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ns32k.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ns32k.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-or1k.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-or1k.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pdp11.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pdp11.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pj.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pj.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ppc.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-ppc.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pru.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-pru.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-riscv.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-riscv.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-rl78.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-rl78.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-rx.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-rx.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-s12z.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-s12z.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-s390.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-s390.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-score.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-score.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-sh.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-sh.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-sparc.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-sparc.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-spu.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-spu.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic30.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic30.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic4x.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic4x.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic54x.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic54x.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic6x.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tic6x.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tilegx.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tilegx.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tilepro.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-tilepro.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-v850.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-v850.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-vax.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-vax.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-visium.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-visium.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-wasm32.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-wasm32.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xgate.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xgate.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xstormy16.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xstormy16.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xtensa.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-xtensa.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-z80.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-z80.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-z8k.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/tc-z8k.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-386bsd.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-aix5.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-armeabi.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-armfbsdeabi.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-armfbsdvfp.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-armlinuxeabi.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-csky_abiv1.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-csky_abiv1_linux.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-csky_abiv2.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-csky_abiv2_linux.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-freebsd.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-generic.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-gnu.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-go32.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-hppa.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-hppa64.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-hppalinux64.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-ia64aix.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-interix.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-lynx.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-macos.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-nbsd.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-nbsd532.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-pc532mach.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-pe.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-solaris.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-svr4.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-tmips.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-vms.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-vxworks.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/te-wince-pe.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/xtensa-relax.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/config/xtensa-relax.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/depend.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/dw2gencfi.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/dw2gencfi.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/dwarf2dbg.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/dwarf2dbg.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/ecoff.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/ecoff.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/ehopt.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/emul-target.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/emul.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/expr.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/expr.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/flonum-copy.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/flonum-konst.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/flonum-mult.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/flonum.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/frags.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/frags.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/hash.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/hash.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/input-file.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/input-file.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/input-scrub.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/itbl-lex.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/itbl-ops.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/itbl-ops.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/listing.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/listing.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/literal.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/macro.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/macro.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/messages.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/obj.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/output-file.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/output-file.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/read.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/read.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/remap.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/sb.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/sb.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/stabs.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/subsegs.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/subsegs.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/symbols.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/symbols.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/tc.h \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/write.c \
     /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/binutils/../../external/gpl3/binutils/dist/gas/write.h

CATALOGS = 
CATOBJEXT = 
INSTOBJEXT = 

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo .msg .cat

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<

.po.cat:
	sed -f ../intl/po2msg.sed < $< > $*.msg \
	  && rm -f $@ && $(GENCAT) $@ $*.msg


all: all-no

all-yes: $(CATALOGS) # $(PACKAGE).pot
all-no:

$(srcdir)/$(PACKAGE).pot: $(POTFILES)
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(top_srcdir) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --msgid-bugs-address=https://sourceware.org/bugzilla/ \
	  --files-from=$(srcdir)/POTFILES.in
	rm -f $(srcdir)/$(PACKAGE).pot
	mv $(PACKAGE).po $(srcdir)/$(PACKAGE).pot

$(srcdir)/cat-id-tbl.c: stamp-cat-id; @:
$(srcdir)/stamp-cat-id: $(PACKAGE).pot
	rm -f cat-id-tbl.tmp
	sed -f ../intl/po2tbl.sed $(srcdir)/$(PACKAGE).pot \
		| sed -e "s/@PACKAGE NAME@/$(PACKAGE)/" > cat-id-tbl.tmp
	if cmp -s cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; then \
	  rm cat-id-tbl.tmp; \
	else \
	  echo cat-id-tbl.c changed; \
	  rm -f $(srcdir)/cat-id-tbl.c; \
	  mv cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; \
	fi
	cd $(srcdir) && rm -f stamp-cat-id && echo timestamp > stamp-cat-id


install: install-exec install-data
install-exec:
install-info:
install-html:
install-pdf:
install-data: install-data-no
install-data-no: all
install-data-yes: all
	if test -r $(MKINSTALLDIRS); then \
	  $(MKINSTALLDIRS) $(DESTDIR)$(datadir); \
	else \
	  $(top_srcdir)/mkinstalldirs $(DESTDIR)$(datadir); \
	fi
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  case "$$cat" in \
	    *.gmo) destdir=$(gnulocaledir);; \
	    *)     destdir=$(localedir);; \
	  esac; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  dir=$(DESTDIR)$$destdir/$$lang/LC_MESSAGES; \
	  if test -r $(MKINSTALLDIRS); then \
	    $(MKINSTALLDIRS) $$dir; \
	  else \
	    $(top_srcdir)/mkinstalldirs $$dir; \
	  fi; \
	  if test -r $$cat; then \
	    $(INSTALL_DATA) $$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $$cat as $$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $(srcdir)/$$cat as" \
		 "$$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  fi; \
	  if test -r $$cat.m; then \
	    $(INSTALL_DATA) $$cat.m $$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	    echo "installing $$cat.m as $$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	  else \
	    if test -r $(srcdir)/$$cat.m ; then \
	      $(INSTALL_DATA) $(srcdir)/$$cat.m \
		$$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	      echo "installing $(srcdir)/$$cat as" \
		   "$$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	    else \
	      true; \
	    fi; \
	  fi; \
	done
	if test "$(PACKAGE)" = "gettext"; then \
	  if test -r $(MKINSTALLDIRS); then \
	    $(MKINSTALLDIRS) $(DESTDIR)$(gettextsrcdir); \
	  else \
	    $(top_srcdir)/mkinstalldirs $(DESTDIR)$(gettextsrcdir); \
	  fi; \
	  $(INSTALL_DATA) $(srcdir)/Makefile.in.in \
			  $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT); \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT).m; \
	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT); \
	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT).m; \
	done
	rm -f $(DESTDIR)$(gettextsrcdir)/po-Makefile.in.in

check: all

cat-id-tbl.o: ../intl/libgettext.h

html dvi pdf ps info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(PACKAGE).po *.old.po cat-id-tbl.tmp
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo *.msg *.cat *.cat.m

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir: update-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(PACKAGE).pot
	PATH=`pwd`/../src:$$PATH; \
	cd $(srcdir); \
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  mv $$lang.po $$lang.old.po; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang.old.po $(PACKAGE).pot -o $$lang.po; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	  fi; \
	done

POTFILES: POTFILES.in
	( if test 'x$(srcdir)' != 'x.'; then \
	    posrcprefix='$(top_srcdir)/'; \
	  else \
	    posrcprefix="../"; \
	  fi; \
	  rm -f $@-t $@ \
	    && (sed -e '/^#/d' -e '/^[ 	]*$$/d' \
		    -e "s@.*@	$$posrcprefix& \\\\@" < $(srcdir)/$@.in \
		| sed -e '$$s/\\$$//') > $@-t \
	    && chmod a-w $@-t \
	    && mv $@-t $@ )

POTFILES.in: # ../Makefile
	cd .. && $(MAKE) po/POTFILES.in

Makefile: Make-in ../config.status POTFILES
	cd .. \
	  && CONFIG_FILES=$(subdir)/Makefile.in:$(subdir)/Make-in \
	     CONFIG_HEADERS= $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
