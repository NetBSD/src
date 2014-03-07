# $NetBSD: Makefile.inc,v 1.4 2014/03/07 04:10:43 matt Exp $

.if ${MACHINE_ARCH} == "powerpc" || ${MLIBDIR} == "powerpc"
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
