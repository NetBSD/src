#	$NetBSD: Makefile,v 1.7 2001/12/28 17:49:32 augustss Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c

LDADD+=	-lusbhid
DPADD+=	${LIBUSB}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
