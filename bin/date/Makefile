#	$NetBSD: Makefile,v 1.15.46.1 2023/06/03 15:27:13 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.init.mk>

PROG=	date
SRCS=	date.c
.if !defined(HOSTPROG)
SRCS+=	netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif
CPPFLAGS+=-I${.CURDIR}

COPTS.date.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
