# $NetBSD: Makefile,v 1.2 2006/07/26 11:00:07 tron Exp $

PROG=		bthcid
MAN=		bthcid.8
SRCS=		bthcid.c hci.c client.c config.c

CPPFLAGS+=	-D_BTHCID_

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBPROP} ${LIBUTIL}
LDADD+=		-lbluetooth -levent -lprop -lutil

.include <bsd.prog.mk>
