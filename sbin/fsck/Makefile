#	$NetBSD: Makefile,v 1.17 2006/08/26 18:14:28 christos Exp $

PROG=	fsck
SRCS=	fsck.c fsutil.c preen.c
MAN=	fsck.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
