#	$NetBSD: Makefile,v 1.13 2006/11/15 16:59:52 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c getdate.y
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${.CURDIR}

.include <bsd.prog.mk>
