#	$NetBSD: Makefile,v 1.6.48.1 2009/08/26 03:35:41 matt Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs,
# and on m68k/powerpc, for kernels with old amigappc bootblocks.
.if (${MACHINE_ARCH} == "m68000" || ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "powerpc")
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
