#	$NetBSD: Makefile,v 1.17 2001/04/14 22:46:26 kleink Exp $

KDIR=	/sys/arch/sparc/include
INCSDIR= /usr/include/sparc

INCS=	ansi.h aout_machdep.h apmvar.h asm.h autoconf.h \
	bswap.h bootinfo.h \
	cdefs.h cgtworeg.h conf.h cpu.h \
	disklabel.h \
	eeprom.h elf_machdep.h endian.h endian_machdep.h \
	float.h frame.h fsr.h \
	idprom.h ieee.h ieeefp.h int_const.h int_mwgwtypes.h int_types.h \
	kbd.h kbio.h kcore.h \
	limits.h lock.h \
	math.h \
	oldmon.h openpromio.h \
	param.h pcb.h pmap.h proc.h profile.h psl.h pte.h ptrace.h \
	reg.h reloc.h \
	setjmp.h signal.h stdarg.h \
	tctrl.h trap.h types.h \
	varargs.h vmparam.h vuid_event.h

SYMLINKS= ../dev/sun/fbio.h ${INCSDIR}/fbio.h

.include <bsd.kinc.mk>
