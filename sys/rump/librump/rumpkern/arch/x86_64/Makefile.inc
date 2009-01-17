#	$NetBSD: Makefile.inc,v 1.4.2.2 2009/01/17 13:29:36 mjf Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
