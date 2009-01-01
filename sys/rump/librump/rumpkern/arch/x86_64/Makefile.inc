#	$NetBSD: Makefile.inc,v 1.3 2009/01/01 19:07:43 pooka Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../amd64/amd64
SRCS+=	kobj_machdep.c
