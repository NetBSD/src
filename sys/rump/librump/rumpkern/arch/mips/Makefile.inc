# $NetBSD: Makefile.inc,v 1.1.24.1 2014/08/20 00:04:41 tls Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
