# $NetBSD: Makefile,v 1.2.26.1 2009/05/13 19:20:19 jym Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c db.c print.c sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lusbhid

.include <bsd.prog.mk>
