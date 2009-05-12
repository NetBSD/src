# $NetBSD: Makefile,v 1.4 2009/05/12 13:12:21 plunky Exp $

USE_FORT?= yes	# network client

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT}
LDADD+=		-lbluetooth -levent

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
