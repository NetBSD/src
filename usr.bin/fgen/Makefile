#	$NetBSD: Makefile,v 1.9 2016/03/08 20:13:44 christos Exp $

CPPFLAGS+= -I${.CURDIR}
PROG=	fgen
SRCS=	fgen.l
MAN=	fgen.1

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
