#	$NetBSD: Makefile,v 1.10.46.2 2009/09/05 03:48:22 matt Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || \
     ((${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb") && \
      ${CPUFLAGS:M-mabi=64} == ""))

PROG=	elf2ecoff

.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
