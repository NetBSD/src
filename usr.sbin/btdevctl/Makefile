# $NetBSD: Makefile,v 1.1 2006/08/13 09:03:23 plunky Exp $

PROG=		btdevctl
MAN=		btdevctl.8
SRCS=		btdevctl.c cfg.c dev.c hid.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBSDP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lsdp -lusbhid

.include <bsd.prog.mk>
