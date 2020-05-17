#	$NetBSD: Makefile,v 1.15 2020/05/17 23:34:11 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ls
SRCS=	cmp.c ls.c main.c print.c util.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.ifdef SMALLPROG
CPPFLAGS+= -DSMALL
.endif

.include <bsd.prog.mk>
