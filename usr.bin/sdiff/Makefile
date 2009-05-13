#	$NetBSD: Makefile,v 1.1.20.1 2009/05/13 19:20:04 jym Exp $

WARNS?=	2	# XXX -Wcast-qual issues

PROG=	sdiff
SRCS=	common.c edit.c sdiff.c strtonum.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
