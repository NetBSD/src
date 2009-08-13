#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 2009/08/13 06:59:37 dholland Exp $

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
