#	$NetBSD: Makefile,v 1.6 2001/12/22 19:39:42 augustss Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c

LDADD+=	-lusb
DPADD+=	${LIBUSB}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
