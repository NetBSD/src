# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:56 gdamore Exp $

PROG=		bthset
SRCS=		bthset.c
MAN=		bthset.1

DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBSDP}
LDADD+=		-lbluetooth -levent -lsdp

.include <bsd.prog.mk>
