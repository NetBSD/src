# $NetBSD: Makefile.inc,v 1.3 2014/02/12 22:28:43 pooka Exp $

.if ${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
