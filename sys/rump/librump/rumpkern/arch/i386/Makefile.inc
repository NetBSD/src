#	$NetBSD: Makefile.inc,v 1.6 2010/06/16 11:45:21 pooka Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
