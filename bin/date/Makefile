#	$NetBSD: Makefile,v 1.15.44.2 2020/04/21 19:37:30 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${.CURDIR}

COPTS.date.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
