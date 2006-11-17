#	$NetBSD: Makefile,v 1.14 2006/11/17 22:11:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${.CURDIR}

.include <bsd.prog.mk>
