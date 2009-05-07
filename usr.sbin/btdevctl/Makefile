# $NetBSD: Makefile,v 1.3 2009/05/07 18:08:55 plunky Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c db.c print.c sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBSDP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lsdp -lusbhid

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
