#	$NetBSD: Makefile,v 1.7.26.1 2007/11/06 23:36:09 matt Exp $

PROG=	patch
SRCS=	patch.c pch.c inp.c version.c util.c backupfile.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
