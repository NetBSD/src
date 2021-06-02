#	$NetBSD: Makefile,v 1.1.10.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	transset
SRCS=	dsimple.c transSet.c

CPPFLAGS+=${X11FLAGS.EXTENSION}

LDADD+=	${XLIBLDADD_X11}
DPADD+=	${XLIBDPADD_X11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
