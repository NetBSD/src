#	$NetBSD: Makefile,v 1.8 2002/02/02 16:50:19 veego Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c

LDADD+=	-lusbhid
DPADD+=	${LIBUSBHIB}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
