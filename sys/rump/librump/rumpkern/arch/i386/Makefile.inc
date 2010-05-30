#	$NetBSD: Makefile.inc,v 1.3.12.1 2010/05/30 05:18:06 rmind Exp $
#

SRCS+=	rumpcpu.c rumpspl.c cpu_counter.c

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
