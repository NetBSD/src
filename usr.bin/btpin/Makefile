# $NetBSD: Makefile,v 1.4 2023/06/03 09:09:16 lukem Exp $

PROG=		btpin
SRCS=		btpin.c
MAN=		btpin.1

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

COPTS.btpin.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
