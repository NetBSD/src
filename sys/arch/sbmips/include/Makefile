# $NetBSD: Makefile,v 1.3.2.2 2002/08/13 03:34:40 simonb Exp $

KDIR=	/sys/arch/sbmips/include
INCSDIR= /usr/include/sbmips

# XXXcgd need local: bus.h pci_machdep.h
# XXXcgd need export: intr.h

INCS=	ansi.h aout_machdep.h asm.h \
	bsd-aout.h bswap.h \
	cdefs.h cpu.h \
	db_machdep.h disklabel.h \
	ecoff_machdep.h elf_machdep.h endian.h endian_machdep.h \
	float.h \
	ieee.h ieeefp.h \
	int_const.h int_fmtio.h int_limits.h int_mwgwtypes.h int_types.h \
	kcore.h kdbparam.h \
	limits.h lock.h locore.h \
	math.h mcontext.h mips_opcode.h \
	param.h pcb.h pmap.h pmc.h proc.h profile.h psl.h pte.h ptrace.h \
	reg.h regdef.h regnum.h reloc.h \
	setjmp.h signal.h stdarg.h \
	trap.h types.h \
	varargs.h vmparam.h

.include <bsd.kinc.mk>
