#	$NetBSD: Makefile,v 1.8 2002/02/23 21:34:06 gmcgarry Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb")

PROG=	elf2ecoff
MAN=	elf2ecoff.1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

.endif
