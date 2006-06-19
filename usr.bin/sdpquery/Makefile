# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c config.c search.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

.include <bsd.prog.mk>
