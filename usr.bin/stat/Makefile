#	$NetBSD: Makefile,v 1.13 2023/06/03 21:30:24 lukem Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MAN=	stat.1 readlink.1
.endif

.include <bsd.own.mk>

# Has fallthru comment that is missed
COPTS.stat.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

COPTS.stat.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
