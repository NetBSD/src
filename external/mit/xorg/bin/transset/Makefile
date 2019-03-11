#	$NetBSD: Makefile,v 1.1 2019/03/11 09:20:13 mrg Exp $

.include <bsd.own.mk>

PROG=	transset
SRCS=	dsimple.c transSet.c

CPPFLAGS+=${X11FLAGS.EXTENSION}

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
