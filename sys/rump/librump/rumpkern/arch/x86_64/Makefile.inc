#	$NetBSD: Makefile.inc,v 1.4.10.1 2010/08/17 06:48:03 uebayasi Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c spinlock.c pmap_x86.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
