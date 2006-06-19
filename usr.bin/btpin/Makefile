# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		btpin
SRCS=		btpin.c
MAN=		btpin.1

CPPFLAGS+=	-I${.CURDIR}/../../usr.sbin/bthcid

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

.include <bsd.prog.mk>
