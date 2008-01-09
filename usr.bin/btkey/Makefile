# $NetBSD: Makefile,v 1.1.4.2 2008/01/09 02:00:31 matt Exp $

PROG=	btkey
SRCS=	btkey.c device.c file.c
MAN=	btkey.1

DPADD+=	${LIBBLUETOOTH} ${LIBPROP}
LDADD+=	-lbluetooth -lprop

.include <bsd.prog.mk>
