# $NetBSD: Makefile,v 1.4 2009/05/12 13:09:43 plunky Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c db.c print.c sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lusbhid

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
