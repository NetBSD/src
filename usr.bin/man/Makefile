#	$NetBSD: Makefile,v 1.9.8.1 2007/11/06 23:36:06 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
