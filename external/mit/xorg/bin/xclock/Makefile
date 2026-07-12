#	$NetBSD: Makefile,v 1.6 2026/07/12 01:23:49 mrg Exp $

.include <bsd.own.mk>

PROG=	xclock
SRCS=	xclock.c Clock.c

CPPFLAGS+=-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=-D_CONST_X_STRING
# Uses uselocale(3) we NetBSD does not have.
CPPFLAGS+=-DNO_I18N
CPPFLAGS+=-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=XClock XClock-color


LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH: ${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
