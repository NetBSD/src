#	$NetBSD: Makefile,v 1.13 2019/01/01 14:01:46 martin Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>	# for MACHINE_CPU
WARNS=6

# Build ELF to aout tools on m68k/earmv4/powerpc,
# for kernels with old bootblocks.
.if (${MACHINE_CPU} == "m68k" || \
     ${MACHINE_CPU} == "arm" || \
     ${MACHINE_CPU} == "powerpc" )
PROG=	elf2aout
.endif

MAN=	elf2aout.1

.include <bsd.prog.mk>
