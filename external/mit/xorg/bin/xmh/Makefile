#	$NetBSD: Makefile,v 1.8.6.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xmh
SRCS=	bbox.c command.c compfuncs.c folder.c init.c main.c \
	menu.c mlist.c msg.c pick.c popup.c screen.c toc.c \
	tocfuncs.c tocutil.c tsource.c util.c viewfuncs.c \
	miscfuncs.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include
CPPFLAGS+=	-DHAS_VFORK -DXVENDORNAME=${XVENDORNAME} \
		-DXORG_RELEASE=${XORG_RELEASE}

APPDEFS=Xmh

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-format

COPTS+=	${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
