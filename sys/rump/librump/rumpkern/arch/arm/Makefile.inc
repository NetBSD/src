# $NetBSD: Makefile.inc,v 1.4.4.3 2017/12/03 11:39:16 jdolecek Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:  ${RUMPTOP}/../arch/arm/arm32
SRCS+=  	kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=  	rump_generic_abi.c rump_generic_cpu.c rump_generic_pmap.c
