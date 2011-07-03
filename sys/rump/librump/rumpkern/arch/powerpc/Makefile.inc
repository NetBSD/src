# $NetBSD: Makefile.inc,v 1.1 2011/07/03 08:53:23 mrg Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32
SRCS+=  	kobj_stubs.c rumpcpu_generic.c
