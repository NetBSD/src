#	$NetBSD: Makefile,v 1.11 2009/12/14 00:43:05 matt Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

ABI64:=  ${CPUFLAGS:M-mabi=64:M-mabi=o64}
# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || \
     ((${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb") && \
      !defined(HOSTPROG) && empty(ABI64)))

PROG=	elf2ecoff

.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
