# $NetBSD: Makefile.inc,v 1.1.12.1 2013/02/25 00:30:09 tls Exp $

.if ${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif
SRCS+=  	kobj_stubs.c rumpcpu_generic.c
