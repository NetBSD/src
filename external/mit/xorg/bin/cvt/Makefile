#	$NetBSD: Makefile,v 1.1 2022/07/14 03:48:34 mrg Exp $

.include <bsd.own.mk>

PROG=	cvt
SRCS=	cvt.c

LDADD+=	-llxcvt
DPADD+=	${LIBXCVT}

.PATH:	${X11SRCDIR.xcvt}/cvt
.PATH:	${X11SRCDIR.xcvt}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
