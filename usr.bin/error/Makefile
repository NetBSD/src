#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.6.14.1 2009/05/13 19:19:49 jym Exp $

WARNS?=	1	# fails -Wcast-qual -Wshadow (etc)

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
