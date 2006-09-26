# $NetBSD: Makefile,v 1.3 2006/09/26 19:18:19 plunky Exp $

PROG=		bthcid
MAN=		bthcid.8
SRCS=		bthcid.c hci.c client.c config.c

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBPROP} ${LIBUTIL}
LDADD+=		-lbluetooth -levent -lprop -lutil

.include <bsd.prog.mk>
