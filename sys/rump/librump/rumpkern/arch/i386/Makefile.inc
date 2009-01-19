#	$NetBSD: Makefile.inc,v 1.3.4.2 2009/01/19 13:20:26 skrll Exp $
#

SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
