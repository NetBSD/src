#	$NetBSD: Makefile,v 1.15.26.1 2020/04/13 08:05:44 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	6

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

COPTS.man.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
