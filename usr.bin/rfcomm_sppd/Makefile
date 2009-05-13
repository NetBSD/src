# $NetBSD: Makefile,v 1.2.20.1 2009/05/13 19:20:02 jym Exp $

USE_FORT?= yes	# network client/server

PROG=		rfcomm_sppd
SRCS=		rfcomm_sppd.c

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

.include <bsd.prog.mk>
