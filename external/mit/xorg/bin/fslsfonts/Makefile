#	$NetBSD: Makefile,v 1.4 2022/10/16 23:38:02 mrg Exp $

.include <bsd.own.mk>

PROG=	fslsfonts

CPPFLAGS+=	-DHAVE_REALLOCARRAY -D_OPENBSD_SOURCE

LDADD+=	-lFS
DPADD+=	${LIBFS}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
