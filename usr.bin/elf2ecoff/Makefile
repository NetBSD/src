#	$NetBSD: Makefile,v 1.10 2003/05/18 07:57:34 lukem Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb")

PROG=	elf2ecoff

.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
