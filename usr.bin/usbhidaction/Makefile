#	$NetBSD: Makefile,v 1.1 2000/12/30 13:07:48 augustss Exp $

.include <bsd.own.mk>

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusb -lutil
DPADD+=	${LIBUSB} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
