#	$NetBSD: Makefile,v 1.9 2002/02/24 01:52:09 thorpej Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb")

PROG=	elf2ecoff

.endif

MAN=	elf2ecoff.1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
