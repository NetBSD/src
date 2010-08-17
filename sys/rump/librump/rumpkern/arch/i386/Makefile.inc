#	$NetBSD: Makefile.inc,v 1.3.10.1 2010/08/17 06:48:02 uebayasi Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
