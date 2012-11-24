# $NetBSD: Makefile.inc,v 1.2 2012/11/24 18:16:15 pgoyette Exp $

.if ${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif
SRCS+=  	kobj_stubs.c rumpcpu_generic.c
