# $NetBSD: Makefile,v 1.2 2006/09/10 15:45:56 plunky Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c db.c print.c sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBSDP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lsdp -lusbhid

.include <bsd.prog.mk>
