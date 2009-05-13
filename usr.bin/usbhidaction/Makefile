#	$NetBSD: Makefile,v 1.5.30.1 2009/05/13 19:20:09 jym Exp $

.include <bsd.own.mk>

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusbhid -lutil
DPADD+=	${LIBUSBHID} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
