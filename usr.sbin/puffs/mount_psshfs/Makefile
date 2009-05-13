#	$NetBSD: Makefile,v 1.3.20.1 2009/05/13 19:20:33 jym Exp $

PROG=	mount_psshfs
SRCS=	psshfs.c fs.c node.c psbuf.c subr.c
LDADD+= -lpuffs -lutil

MAN=	mount_psshfs.8

.include <bsd.prog.mk>
