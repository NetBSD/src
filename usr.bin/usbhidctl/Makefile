#	$NetBSD: Makefile,v 1.5 2001/12/22 12:34:41 augustss Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c

LDADD+=	-lusbhid
DPADD+=	${LIBUSB}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
