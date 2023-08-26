#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11 2023/08/26 14:50:53 rillig Exp $

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

LINTFLAGS+=	-w		# treat warnings as errors
LINTFLAGS+=	-T		# strict bool mode

.include <bsd.prog.mk>
