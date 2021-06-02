#	$NetBSD: Makefile,v 1.2.24.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xkbprint
SRCS=	xkbprint.c utils.c psgeom.c

CPPFLAGS+=	-DHAVE_ASPRINTF

LDADD+=	${XLIBLDADD_KDFILE}
DPADD+=	${XLIBDPADD_KDFILE}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

# XXX
CPPFLAGS.psgeom.c+=	-Wno-error

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
