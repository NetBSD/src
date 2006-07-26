# $NetBSD: Makefile,v 1.2 2006/07/26 10:54:49 tron Exp $

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c search.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

.include <bsd.prog.mk>
