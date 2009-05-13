# $NetBSD: Makefile,v 1.3.20.1 2009/05/13 19:20:04 jym Exp $

USE_FORT?= yes	# network client

PROG=		sdpquery
MAN=		sdpquery.1
SRCS=		sdpquery.c command.c print.c

DPADD+=		${LIBBLUETOOTH}
LDADD+=		-lbluetooth

.include <bsd.prog.mk>
