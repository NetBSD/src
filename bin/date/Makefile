#	$NetBSD: Makefile,v 1.9 1997/07/20 22:36:42 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
