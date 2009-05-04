#	$NetBSD: Makefile.inc,v 1.4.8.2 2009/05/04 08:14:30 yamt Exp $
#

.PATH:	${ARCHDIR}/../i386
SRCS+=	rumpcpu.c rumpspl.c

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
