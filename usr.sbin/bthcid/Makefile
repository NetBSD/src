# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		bthcid
MAN=		bthcid.8
SRCS=		bthcid.c hci.c client.c config.c

CPPFLAGS+=	-D_BTHCID_

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=		-lbluetooth -levent -lutil

.include <bsd.prog.mk>
