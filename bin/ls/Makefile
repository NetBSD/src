#	$NetBSD: Makefile,v 1.14.78.2 2020/04/21 19:37:33 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ls
SRCS=	cmp.c ls.c main.c print.c util.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
