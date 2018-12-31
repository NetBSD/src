#	$NetBSD: Makefile,v 1.12 2018/12/31 13:32:05 christos Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>	# for MACHINE_CPU
WARNS=6

# Build ELF to aout tools on m68k/earmv4/powerpc,
# for kernels with old bootblocks.
.if (${MACHINE_CPU} == "m68k" || \
     ${MACHINE_ARCH} == "arm" || \
     ${MACHINE_ARCH} == "powerpc" || \
     ${MACHINE_ARCH} == "powerpc64")
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
