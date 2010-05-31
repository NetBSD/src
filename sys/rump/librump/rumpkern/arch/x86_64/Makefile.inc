#	$NetBSD: Makefile.inc,v 1.6 2010/05/31 22:31:07 pooka Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
