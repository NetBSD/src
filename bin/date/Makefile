#	$NetBSD: Makefile,v 1.8 1997/07/20 05:18:14 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
WARNS=1

.include <bsd.prog.mk>
