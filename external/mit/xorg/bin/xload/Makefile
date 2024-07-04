#	$NetBSD: Makefile,v 1.7 2024/07/04 00:34:54 mrg Exp $

.include <bsd.own.mk>

PROG=	xload
SRCS=	xload.c get_load.c get_rload.c

CPPFLAGS+=${X11FLAGS.VERSION} -DHAVE_GETLOADAVG \
	-DHAVE_ASPRINTF \
	-DHAVE_PROTOCOLS_RWHOD_H \
	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"
CPPFLAGS+=-DUSE_GETTEXT -DLOCALEDIR=\"${LOCALEDIR}\"

APPDEFS=XLoad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lintl
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBINTL}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
