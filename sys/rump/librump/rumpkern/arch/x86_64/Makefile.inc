#	$NetBSD: Makefile.inc,v 1.5 2010/05/11 21:03:41 pooka Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
