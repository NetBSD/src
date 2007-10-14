#	$NetBSD: Makefile,v 1.8 2007/10/14 04:54:34 lukem Exp $

PROG=	patch
SRCS=	patch.c pch.c inp.c version.c util.c backupfile.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
