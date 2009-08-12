#	$NetBSD: Makefile,v 1.7 2009/08/12 23:39:13 matt Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

# Build ELF to {ecoff, aout} tools on m68k/powerpc, for kernels with old amigappc bootblocks.
.if (${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "powerpc")
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
