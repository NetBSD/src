#	$NetBSD: Makefile,v 1.14 2024/01/29 21:55:24 christos Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MAN=	stat.1 readlink.1
.endif

.include <bsd.own.mk>

# Has fallthru comment that is missed
COPTS.stat.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

COPTS.stat.c += -Wno-format-nonliteral

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
