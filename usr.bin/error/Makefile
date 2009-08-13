#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 2009/08/13 04:09:53 dholland Exp $

WARNS?=	2	# fails -Wcast-qual -Wshadow (etc)

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
