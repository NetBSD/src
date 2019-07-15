#	$NetBSD: Makefile,v 1.9 2019/07/15 05:41:17 mrg Exp $

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

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
