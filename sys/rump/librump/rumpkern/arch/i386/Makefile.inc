#	$NetBSD: Makefile.inc,v 1.3.12.2 2010/07/03 01:20:03 rmind Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
