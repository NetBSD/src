#	$NetBSD: Makefile,v 1.5 2022/10/28 09:43:58 wiz Exp $

.include <bsd.own.mk>

PROG=	fslsfonts

CPPFLAGS+=	-DHAVE_REALLOCARRAY

LDADD+=	-lFS
DPADD+=	${LIBFS}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
