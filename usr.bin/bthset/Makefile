# $NetBSD: Makefile,v 1.3 2009/05/07 18:08:55 plunky Exp $

USE_FORT?= yes	# network client

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBSDP}
LDADD+=		-lbluetooth -levent -lsdp

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
