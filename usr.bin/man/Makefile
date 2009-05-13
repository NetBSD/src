#	$NetBSD: Makefile,v 1.10.14.1 2009/05/13 19:19:57 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=	2	# XXX -Wcast-qual issues

PROG=	man
SRCS=	man.c manconf.c
MAN=	man.1 man.conf.5

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
