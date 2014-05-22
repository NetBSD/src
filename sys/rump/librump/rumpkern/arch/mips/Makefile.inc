# $NetBSD: Makefile.inc,v 1.1.14.1 2014/05/22 11:41:15 yamt Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
