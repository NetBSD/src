#	$NetBSD: Makefile,v 1.4 1999/06/09 02:54:17 simonb Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb")
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
