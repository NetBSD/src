# $NetBSD: Makefile,v 1.3 2019/10/13 07:28:14 mrg Exp $

PROG=		btpin
SRCS=		btpin.c
MAN=		btpin.1

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

COPTS.btpin.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
