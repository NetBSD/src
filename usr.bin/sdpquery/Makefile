# $NetBSD: Makefile,v 1.3 2007/05/28 12:06:31 tls Exp $

USE_FORT?= yes	# network client

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c search.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

.include <bsd.prog.mk>
