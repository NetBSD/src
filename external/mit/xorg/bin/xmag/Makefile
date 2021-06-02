#	$NetBSD: Makefile,v 1.4.24.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xmag
SRCS=	xmag.c CutPaste.c RootWin.c Scale.c

APPDEFS=Xmag

CPPFLAGS+=	-DHAVE_LRINT -DHAVE_NANOSLEEP

LDADD+=	${XLIBLDADD_XAW} -lm
DPADD+=	${XLIBDPADD_XAW} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
