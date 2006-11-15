#	$NetBSD: Makefile,v 1.12 2006/11/15 16:55:18 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c getdate.y
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
