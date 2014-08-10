# $NetBSD: Makefile.inc,v 1.2.2.1 2014/08/10 06:56:51 tls Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:  ${RUMPTOP}/../arch/arm/arm32
SRCS+=  	kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=  	rump_generic_cpu.c rump_generic_pmap.c
