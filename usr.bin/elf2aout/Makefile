#	$NetBSD: Makefile,v 1.5 2000/05/21 06:48:08 is Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs,
# and on m68k/powerpc, for kernels with old amigappc bootblocks.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "powerpc")
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
