#	$NetBSD: Makefile,v 1.11 2003/07/18 02:18:57 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
