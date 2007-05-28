# $NetBSD: Makefile,v 1.4 2007/05/28 12:06:34 tls Exp $

USE_FORT?= yes	# network server

PROG=		bthcid
MAN=		bthcid.8
SRCS=		bthcid.c hci.c client.c config.c

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBPROP} ${LIBUTIL}
LDADD+=		-lbluetooth -levent -lprop -lutil

.include <bsd.prog.mk>
