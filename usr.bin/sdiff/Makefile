#	$NetBSD: Makefile,v 1.1 2007/02/18 22:13:42 rmind Exp $

PROG=	sdiff
SRCS=	common.c edit.c sdiff.c strtonum.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS=	1

.include <bsd.prog.mk>
