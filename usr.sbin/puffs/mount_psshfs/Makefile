#	$NetBSD: Makefile,v 1.1 2006/12/29 15:35:39 pooka Exp $

PROG=	mount_psshfs
SRCS=	psshfs.c fs.c node.c psbuf.c subr.c
LDADD+= -lpuffs -lutil
DBG=-g -O0
WARNS=2

NOMAN=	time waits for

.include <bsd.prog.mk>
