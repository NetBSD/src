# $NetBSD: Makefile,v 1.2 2006/09/26 19:18:19 plunky Exp $

PROG=		btpin
SRCS=		btpin.c
MAN=		btpin.1

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

.include <bsd.prog.mk>
