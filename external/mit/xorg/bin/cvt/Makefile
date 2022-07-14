#	$NetBSD: Makefile,v 1.2 2022/07/14 07:41:49 mrg Exp $

.include <bsd.own.mk>

PROG=	cvt
SRCS=	cvt.c

LDADD+=	-lxcvt -lm
DPADD+=	${LIBXCVT} ${LIBM} 

.PATH:	${X11SRCDIR.xcvt}/cvt
.PATH:	${X11SRCDIR.xcvt}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
