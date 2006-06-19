# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		btcontrol
MAN=		btcontrol.8
SRCS=		btcontrol.c cfg.c dev.c hid.c

DPADD+=		${LIBBLUETOOTH} ${LIBUSBHID}
LDADD+=		-lbluetooth -lusbhid

.include <bsd.prog.mk>
