#	$NetBSD: Makefile,v 1.10 2016/03/10 11:32:10 joerg Exp $

CPPFLAGS+= -I${.CURDIR}
PROG=	fgen
SRCS=	fgen.l
MAN=	fgen.1

.if !defined(HOSTPROG)
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

.include <bsd.prog.mk>
