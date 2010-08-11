#	$NetBSD: Makefile.inc,v 1.4.8.3 2010/08/11 22:55:07 yamt Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
