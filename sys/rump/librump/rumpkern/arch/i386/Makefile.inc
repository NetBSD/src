#	$NetBSD: Makefile.inc,v 1.3.2.2 2009/01/17 13:29:36 mjf Exp $
#

SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
