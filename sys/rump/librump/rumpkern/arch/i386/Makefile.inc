#	$NetBSD: Makefile.inc,v 1.3.8.2 2009/05/04 08:14:30 yamt Exp $
#

SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
