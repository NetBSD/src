#	$NetBSD: Makefile,v 1.13 2013/07/18 16:28:52 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	6

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

COPTS.man.c += -Wno-format

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
