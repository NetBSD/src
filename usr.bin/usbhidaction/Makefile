#	$NetBSD: Makefile,v 1.2 2001/12/28 17:49:31 augustss Exp $

.include <bsd.own.mk>

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusbhid -lutil
DPADD+=	${LIBUSB} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
