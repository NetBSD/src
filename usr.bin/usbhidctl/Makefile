#	$NetBSD: Makefile,v 1.9 2002/02/02 16:54:28 veego Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c

LDADD+=	-lusbhid
DPADD+=	${LIBUSBHID}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
