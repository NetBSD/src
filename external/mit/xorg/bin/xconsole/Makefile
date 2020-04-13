#	$NetBSD: Makefile,v 1.4.26.1 2020/04/13 08:02:30 martin Exp $

.include <bsd.own.mk>

PROG=	xconsole

APPDEFS=XConsole

LDADD+=	-lutil -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBUTIL} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

CPPFLAGS+=-DHAS_OPENPTY -DHAVE_UTIL_H

COPTS.xconsole.c+=		${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
