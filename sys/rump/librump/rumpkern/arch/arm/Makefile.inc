# $NetBSD: Makefile.inc,v 1.1 2013/08/08 06:50:24 matt Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32

.PATH:  ${RUMPTOP}/../arch/arm/arm32
SRCS+=  	kobj_machdep.c
SRCS+=  	rumpcpu_generic.c
