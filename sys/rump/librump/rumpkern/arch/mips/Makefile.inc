# $NetBSD: Makefile.inc,v 1.1.28.1 2014/05/18 17:46:18 rmind Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
