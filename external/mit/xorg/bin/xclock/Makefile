#	$NetBSD: Makefile,v 1.5 2019/07/15 05:41:17 mrg Exp $

.include <bsd.own.mk>

PROG=	xclock
SRCS=	xclock.c Clock.c

CPPFLAGS+=-DXRENDER -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=-D_CONST_X_STRING
CPPFLAGS+=-I${DESTDIR}${X11INCDIR}/freetype2/freetype

APPDEFS=XClock XClock-color


LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+= ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH: ${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
