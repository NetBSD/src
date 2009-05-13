# $NetBSD: Makefile,v 1.2.20.1 2009/05/13 19:19:44 jym Exp $

USE_FORT?= yes	# network client

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT}
LDADD+=		-lbluetooth -levent

.include <bsd.prog.mk>
