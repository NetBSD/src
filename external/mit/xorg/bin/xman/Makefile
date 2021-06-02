#	$NetBSD: Makefile,v 1.9.8.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xman
SRCS=	ScrollByL.c handler.c man.c buttons.c help.c \
	search.c globals.c main.c misc.c tkfuncs.c vendor.c

CPPFLAGS+=${X11FLAGS.VERSION} -DHAVE_MKSTEMP
CPPFLAGS+=-DHELPFILE=\"${X11LIBDIR}/xman.help\"
CPPFLAGS+=-DMANCONFIGSTYLE_OpenBSD -DMANCONF=\"/etc/man.conf\"
CPPFLAGS+=-D_CONST_X_STRING

APPDEFS=Xman

FILES=	xman.help
FILESDIR=${X11LIBDIR}

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
