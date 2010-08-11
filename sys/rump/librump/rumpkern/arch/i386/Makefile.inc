#	$NetBSD: Makefile.inc,v 1.3.8.3 2010/08/11 22:55:07 yamt Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
