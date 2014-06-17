# $NetBSD: Makefile.inc,v 1.3 2014/06/17 06:31:47 alnsn Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

SRCS+=		cpufunc.c

.PATH:  ${RUMPTOP}/../arch/arm/arm32
SRCS+=  	kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=  	rump_generic_cpu.c rump_generic_pmap.c
