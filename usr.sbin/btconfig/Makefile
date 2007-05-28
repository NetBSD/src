# $NetBSD: Makefile,v 1.2 2007/05/28 12:06:34 tls Exp $

USE_FORT?= yes	#network client

PROG=		btconfig
SRCS=		btconfig.c
MAN=		btconfig.8

DPADD+=		${LIBBLUETOOTH} ${LIBUTIL}
LDADD+=		-lbluetooth -lutil

.include <bsd.prog.mk>
