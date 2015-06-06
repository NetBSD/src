# $NetBSD: Makefile.inc,v 1.2.6.1 2015/06/06 14:40:29 skrll Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_abi.c
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
