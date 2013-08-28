# $NetBSD: Makefile.inc,v 1.1.2.2 2013/08/28 23:59:37 rmind Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:  ${RUMPTOP}/../arch/arm/arm32
SRCS+=  	kobj_machdep.c
SRCS+=  	rumpcpu_generic.c
