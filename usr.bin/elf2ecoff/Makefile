#	$NetBSD: Makefile,v 1.7 1999/06/09 02:54:18 simonb Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb")
PROG=	elf2ecoff
.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
