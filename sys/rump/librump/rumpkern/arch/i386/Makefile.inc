#	$NetBSD: Makefile.inc,v 1.5 2010/05/31 22:31:07 pooka Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
