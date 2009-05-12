# $NetBSD: Makefile,v 1.5 2009/05/12 13:15:10 plunky Exp $

USE_FORT?= yes	# network client

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c search.c

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
