# $NetBSD: Makefile,v 1.2 2006/07/26 10:31:00 tron Exp $

PROG=		btcontrol
MAN=		btcontrol.8
SRCS=		btcontrol.c cfg.c dev.c hid.c

DPADD+=		${LIBBLUETOOTH} ${LIBPROP} ${LIBSDP} ${LIBUSBHID}
LDADD+=		-lbluetooth -lprop -lsdp -lusbhid

.include <bsd.prog.mk>
