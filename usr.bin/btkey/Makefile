# $NetBSD: Makefile,v 1.1 2007/11/09 21:18:24 plunky Exp $

PROG=	btkey
SRCS=	btkey.c device.c file.c
MAN=	btkey.1

DPADD+=	${LIBBLUETOOTH} ${LIBPROP}
LDADD+=	-lbluetooth -lprop

.include <bsd.prog.mk>
