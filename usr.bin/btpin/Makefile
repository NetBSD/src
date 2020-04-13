# $NetBSD: Makefile,v 1.2.82.1 2020/04/13 08:05:41 martin Exp $

PROG=		btpin
SRCS=		btpin.c
MAN=		btpin.1

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

COPTS.btpin.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
