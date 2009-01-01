#	$NetBSD: Makefile.inc,v 1.3 2009/01/01 19:07:43 pooka Exp $
#

SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
