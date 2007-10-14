#	$NetBSD: Makefile,v 1.3 2007/10/14 11:21:32 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	pr
SRCS=	pr.c egetopt.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
