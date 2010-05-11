#	$NetBSD: Makefile.inc,v 1.4 2010/05/11 21:03:41 pooka Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
