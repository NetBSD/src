#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.6 2007/10/05 07:27:42 lukem Exp $

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
