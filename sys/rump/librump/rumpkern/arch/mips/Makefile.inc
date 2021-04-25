# $NetBSD: Makefile.inc,v 1.4 2021/04/25 15:12:02 christos Exp $

.if empty(MACHINE_ARCH:Mmipsn64*)
CPPFLAGS+=	-DARCH_ELFSIZE=32
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_abi.c
SRCS+= 	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
