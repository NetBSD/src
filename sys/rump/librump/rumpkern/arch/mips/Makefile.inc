# $NetBSD: Makefile.inc,v 1.1 2009/12/14 21:19:23 matt Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32
SRCS+=  	kobj_stubs.c rumpcpu_generic.c

