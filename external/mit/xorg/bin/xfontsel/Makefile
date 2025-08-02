#	$NetBSD: Makefile,v 1.3.40.1 2025/08/02 05:50:39 perseant Exp $

.include <bsd.own.mk>

PROG=	xfontsel
SRCS=	xfontsel.c ULabel.c

APPDEFS=XFontSel

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lintl
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBINTL}

CPPFLAGS+=	-DUSE_GETTEXT -DLOCALEDIR=\"${LOCALEDIR}\"
CPPFLAGS+=	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
