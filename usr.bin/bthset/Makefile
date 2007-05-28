# $NetBSD: Makefile,v 1.2 2007/05/28 12:06:24 tls Exp $

USE_FORT?= yes	# network client

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBSDP}
LDADD+=		-lbluetooth -levent -lsdp

.include <bsd.prog.mk>
