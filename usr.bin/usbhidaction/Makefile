#	$NetBSD: Makefile,v 1.4 2002/02/02 16:54:26 veego Exp $

.include <bsd.own.mk>

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusbhid -lutil
DPADD+=	${LIBUSBHID} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
