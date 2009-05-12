# $NetBSD: Makefile,v 1.5 2009/05/12 18:39:55 plunky Exp $

USE_FORT?= yes	# network client

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT}
LDADD+=		-lbluetooth -levent

.include <bsd.prog.mk>
