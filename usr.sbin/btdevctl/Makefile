# $NetBSD: Makefile,v 1.5 2009/05/12 18:39:20 plunky Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c db.c print.c sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lusbhid

.include <bsd.prog.mk>
