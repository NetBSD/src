#	$NetBSD: Makefile,v 1.10.46.1 2009/08/26 03:35:41 matt Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || \
     ${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb")

PROG=	elf2ecoff

.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
