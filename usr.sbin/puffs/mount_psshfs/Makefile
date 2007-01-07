#	$NetBSD: Makefile,v 1.2 2007/01/07 19:56:26 pooka Exp $

PROG=	mount_psshfs
SRCS=	psshfs.c fs.c node.c psbuf.c subr.c
LDADD+= -lpuffs -lutil
DBG=-g -O0
WARNS=2

MAN=	mount_psshfs.8

.include <bsd.prog.mk>
