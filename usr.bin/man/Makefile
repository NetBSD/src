#	$NetBSD: Makefile,v 1.11.6.1 2014/05/22 11:42:46 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	6

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
