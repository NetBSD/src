# $NetBSD: Makefile.inc,v 1.1.24.2 2017/12/03 11:39:16 jdolecek Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_abi.c
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
