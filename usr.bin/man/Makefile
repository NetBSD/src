#	$NetBSD: Makefile,v 1.11.16.1 2013/07/23 21:07:39 riastradh Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	6

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

COPTS.man.c += -Wno-format-nonliteral

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
