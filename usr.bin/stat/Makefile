#	$NetBSD: Makefile,v 1.15 2024/01/29 22:01:58 christos Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MAN=	stat.1 readlink.1
.endif

.include <bsd.own.mk>

# Has fallthru comment that is missed
COPTS.stat.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

COPTS.stat.c += -Wno-format-nonliteral

.if !defined(HOSTPROG)
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

.include <bsd.prog.mk>
