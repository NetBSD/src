#	$NetBSD: Makefile.inc,v 1.4.4.2 2009/01/19 13:20:26 skrll Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
