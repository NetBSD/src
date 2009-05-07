# $NetBSD: Makefile,v 1.4 2009/05/07 18:12:08 plunky Exp $

USE_FORT?= yes	# network client

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c search.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
