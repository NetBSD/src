#	$NetBSD: Makefile,v 1.10 2001/07/03 21:50:13 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
NOHTML=doc2html-error

.include <bsd.prog.mk>
