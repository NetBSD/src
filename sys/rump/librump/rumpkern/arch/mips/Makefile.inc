# $NetBSD: Makefile.inc,v 1.1.4.2 2010/03/11 15:04:38 yamt Exp $

CPPFLAGS+=	-DARCH_ELFSIZE=32
SRCS+=  	kobj_stubs.c rumpcpu_generic.c

