#	$NetBSD: Makefile,v 1.12 2011/07/10 08:43:50 mrg Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

# Build ELF to ecoff tools on mips, for old bootblocks/PROMs.
.if ${MACHINE_CPU} == "mips"
PROG=	elf2ecoff
.endif

MAN=	elf2ecoff.1

.include <bsd.prog.mk>
