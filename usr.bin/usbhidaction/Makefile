#	$NetBSD: Makefile,v 1.3 2002/02/02 16:50:19 veego Exp $

.include <bsd.own.mk>

PROG=	usbhidaction
SRCS=	usbhidaction.c

LDADD+=	-lusbhid -lutil
DPADD+=	${LIBUSBHIB} ${LIBUTIL}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
