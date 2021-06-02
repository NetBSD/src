#	$NetBSD: Makefile,v 1.5.6.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	xconsole

APPDEFS=XConsole

LDADD+=	-lutil ${XLIBLDADD_XAW}
DPADD+=	${LIBUTIL} ${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

CPPFLAGS+=-DHAS_OPENPTY -DHAVE_UTIL_H

COPTS.xconsole.c+=		${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
