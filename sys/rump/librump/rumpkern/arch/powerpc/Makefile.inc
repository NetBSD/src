# $NetBSD: Makefile.inc,v 1.1.2.1 2013/01/16 05:33:52 yamt Exp $

.if ${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif
SRCS+=  	kobj_stubs.c rumpcpu_generic.c
